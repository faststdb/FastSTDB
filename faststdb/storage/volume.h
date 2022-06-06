/**
 * \file volume.h
 *
 */
#ifndef FASTSTDB_STORAGE_VOLUME_H_
#define FASTSTDB_STORAGE_VOLUME_H_

#include <cstdint>
#include <future>
#include <memory>

#include <limits>
#include <vector>

#include "faststdb/common/basic.h"
#include "faststdb/common/mmapfile.h"
#include "faststdb/common/status.h"
#include "faststdb/storage/volume_registry.h"

namespace faststdb {
namespace storage {

//! Address of the block inside storage
typedef u64 LogicAddr;

//! This value represents empty addr. It's too large to be used as a real block addr.
static const LogicAddr EMPTY_ADDR = std::numeric_limits<LogicAddr>::max();

//! Address of the block inside volume (index of the block)
typedef u32 BlockAddr;
enum { FASTSTDB_BLOCK_SIZE = 4096 };

struct IOVecBlock {
  enum {
    NCOMPONENTS = 4,
    COMPONENT_SIZE = FASTSTDB_BLOCK_SIZE / NCOMPONENTS,
  };

  std::vector<u8>  data_[NCOMPONENTS];
  int pos_;  //! write pos
  LogicAddr addr_;

  /**
   * @brief Create empty IOVecBlock
   * All storage components wouldn't be allocated, pos_
   * will be set to 0.
   */
  IOVecBlock();

  /**
   * @brief Create allocated IOVecBlock
   * FASTSTDB_BLOCK_SIZE bytes will be allocated for the first storage
   * component, pos_ will be set to FASTSTDB_BLOCK_SIZE.
   * The parameter value doesn't actually matter (used to distingwish between the c-tor's).
   * The block is not writable. Methods `get` and `get_raw` will work.
   */
  IOVecBlock(bool);

  /** Add component if block is less than NCOMPONENTS in size.
   *  Return index of the component or -1 if block is full.
   */
  int add();

  void set_addr(LogicAddr addr);

  LogicAddr get_addr() const;

  int space_left() const;

  //! Remaining bytes to read from offset to current write pos_
  int bytes_to_read(u32 offset) const;

  int size() const;

  void put(u8 val);

  u8 get(u32 offset) const;

  bool safe_put(u8 val);

  int get_write_pos() const;

  void set_write_pos(int pos);

  template<class POD>
  void put(const POD& data) {
    const u8* it = reinterpret_cast<const u8*>(&data);
    for (u32 i = 0; i < sizeof(POD); i++) {
      put(it[i]);
    }
  }
  
  template<class POD>
  POD get_raw(u32 offset) const {
    const u32 sz = sizeof(POD);
    union {
      POD retval;
      u8 bits[sz];
    } raw;
    for (u32 i = 0; i < sz; i++) {
      raw.bits[i] = get(offset + i);
    }
    return raw.retval;
  }

  //! Allocate memory inside the stream (at the current write position)
  template<class POD>
  POD* allocate() {
    int c = pos_ / COMPONENT_SIZE;
    int i = pos_ % COMPONENT_SIZE;
    if (c >= NCOMPONENTS) {
      return nullptr;
    }
    if (data_[c].empty()) {
      data_[c].resize(COMPONENT_SIZE);
    }
    if ((data_[c].size() - static_cast<u32>(i)) < sizeof(POD)) {
      return nullptr;
    }
    POD* result = reinterpret_cast<POD*>(data_[c].data() + i);
    pos_ += sizeof(POD);
    return result;
  }
  
  //! Allocate memory inside the stream (at the current write position)
  u8* allocate(u32 size);
  
  bool is_readonly() const;

  const u8* get_data(int component) const;

  const u8* get_cdata(int component) const;

  template<typename Header>
  const Header* get_header() const {
    static_assert(sizeof(Header) < 1024, "Header should be less than 1KB");
    const u8* ptr = get_data(0);
    return reinterpret_cast<const Header*>(ptr);
  }
  
  template<typename Header>
  const Header* get_cheader() const {
    return get_header<Header>();
  }

  template<typename Header>
  Header* get_header() {
    static_assert(sizeof(Header) < 1024, "Header should be less than 1KB");
    u8* ptr = get_data(0);
    return reinterpret_cast<Header*>(ptr);
  }

  u8* get_data(int component);

  size_t get_size(int component) const;

  /** Copy content of the 'other' block into current block.
  */
  void copy_from(const IOVecBlock& other);

  /** Copy 'size' bytes into 'dest' starting from 'offset'.
   * Return number of copied bytes.
   */
  u32 read_chunk(void *dest, u32 offset, u32 size);

  /** Copy 'size' bytes into the block starting from 'source'.
   * Return number of new write pos or 0 on error.
   */
  u32 append_chunk(const void *source, u32 size);

  /** Adjust write pos.
   * Try to shrink the block by deallocating unused chunks.
   */
  void set_write_pos_and_shrink(int top);
};

/** Class that represents metadata volume.
 * MetaVolume is a file that contains some information
 * about each regullar volume - write position, generation, etc.
 *
 * Hardware asumptions. At this point we assume that disck sector size is
 * 4KB and sector writes are atomic (each write less or equal to 4K will be
 * fully written to disk or not, FS checksum failure is a hardware bug, not
 * a result of the partial sector write).
 */
class MetaVolume {
  std::shared_ptr<VolumeRegistry>  meta_;
  size_t                           file_size_;
  mutable std::vector<u8>          double_write_buffer_;
  const std::string                path_;
  
  MetaVolume(std::shared_ptr<VolumeRegistry> meta);
 
 public:
  /** Open existing meta-volume.
   * @param path Path to meta-volume.
   * @throw std::runtime_error on error.
   * @return new MetaVolume instance.
   */
  static std::unique_ptr<MetaVolume> open_existing(std::shared_ptr<VolumeRegistry> meta);

  //! Get number of blocks in the volume.
  std::tuple<common::Status, u32> get_nblocks(u32 id) const;

  //! Get total capacity of the volume.
  std::tuple<common::Status, u32> get_capacity(u32 id) const;

  //! Get volume's generation.
  std::tuple<common::Status, u32> get_generation(u32 id) const;

  size_t get_nvolumes() const;

  /**
   * @brief Adds new tracked volume
   * @param id is a new volume's id
   * @param vol_capacity is a volume's capacity
   * @return status
   */
  common::Status add_volume(u32 id, u32 vol_capacity, const std::string &path);

  common::Status update(u32 id, u32 nblocks, u32 capacity, u32 gen);

  //! Set number of used blocks for the volume.
  common::Status set_nblocks(u32 id, u32 nblocks);

  //! Set volume capacity
  common::Status set_capacity(u32 id, u32 nblocks);

  //! Set generation
  common::Status set_generation(u32 id, u32 nblocks);

  //! Flush entire file
  void flush();

  //! Flush one entry
  common::Status flush(u32 id);
};

class Volume {
 protected:
  u32         file_size_;
  u32         write_pos_;

  std::string path_;

  std::unique_ptr<common::MMapFile> mmap_;
  u8* mmap_ptr_;

  Volume(const char* path, size_t write_pos);

 public:
  /**
   * Create a new file.
   * @param path Path to volum file
   * @param capacity the capacity of volume 
   */
  static void create_new(const char* path, u64 capacity);

  /** Open volume.
   * @throw std::runtime_error on error.
   * @param path Path to volume file.
   * @param pos Write position inside volume (in blocks).
   * @return New instance of Volume.
   */
  static std::unique_ptr<Volume> open_existing(const char* path, u64 pos);

  void reset();

  //! Append block to file (source size should be 4 at least BLOCK_SIZE)
  std::tuple<common::Status, BlockAddr> append_block(const u8* source);

  std::tuple<common::Status, BlockAddr> append_block(const IOVecBlock* source);

  //! Flush volume
  void flush();

  //! Read fixed size block from file
  common::Status read_block(u32 ix, u8* dest) const;

  //! Read fixed size block from file
  std::tuple<common::Status, std::unique_ptr<IOVecBlock>> read_block(u32 ix) const;

  /**
   * @brief Read block without copying the data (only works if mmap available)
   * @param ix is an index of the page
   * @return status
   */
  std::tuple<common::Status, const u8*> read_block_zero_copy(u32 ix) const;

  //! Return size in blocks
  u32 get_size() const;

  //! Return path of volume
  std::string get_path() const;
};

}  // namespace storage
}  // namespace faststdb

#endif  // FASTSTDB_STORAGE_VOLUME_H_
