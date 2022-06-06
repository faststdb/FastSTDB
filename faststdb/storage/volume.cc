/**
 * \file volume.cc
 */

#include "faststdb/storage/volume.h"

#include <sys/uio.h>

#include <mutex>
#include <set>
#include <unordered_map>

#include "faststdb/common/logging.h"

namespace faststdb {
namespace storage {

IOVecBlock::IOVecBlock() :
    data_{},
    pos_(0),
    addr_(EMPTY_ADDR) { }

IOVecBlock::IOVecBlock(bool) :
    data_{},
    pos_(FASTSTDB_BLOCK_SIZE),
    addr_(EMPTY_ADDR) {
  data_[0].resize(FASTSTDB_BLOCK_SIZE);
}

void IOVecBlock::set_addr(LogicAddr addr) {
  addr_ = addr;
}

LogicAddr IOVecBlock::get_addr() const {
  return addr_;
}

bool IOVecBlock::is_readonly() const {
  return false;
}

int IOVecBlock::add() {
  for (int i = 0; i < NCOMPONENTS; i++) {
    if (data_[i].size() == 0) {
      data_[i].resize(COMPONENT_SIZE);
      return i;
    }
  }
  return -1;
}

int IOVecBlock::space_left() const {
  return FASTSTDB_BLOCK_SIZE - pos_;
}

int IOVecBlock::bytes_to_read(u32 offset) const {
  return pos_ - offset;
}

int IOVecBlock::size() const {
  return pos_;
}

void IOVecBlock::put(u8 val) {
  int c = pos_ / COMPONENT_SIZE;
  int i = pos_ % COMPONENT_SIZE;
  if (data_[c].empty()) {
    data_[c].resize(COMPONENT_SIZE);
  }
  data_[c][static_cast<size_t>(i)] = val;
  pos_++;
}

u8* IOVecBlock::allocate(u32 size) {
  int c = pos_ / COMPONENT_SIZE;
  int i = pos_ % COMPONENT_SIZE;
  if (c >= NCOMPONENTS) {
    return nullptr;
  }
  if (data_[c].empty()) {
    data_[c].resize(COMPONENT_SIZE);
  }
  if ((data_[c].size() - static_cast<u32>(i)) < size) {
    return nullptr;
  }
  u8* result = data_[c].data() + i;
  pos_ += size;
  return result;
}

u8 IOVecBlock::get(u32 offset) const {
  u32 c;
  u32 i;
  if (data_[0].size() == FASTSTDB_BLOCK_SIZE) {
    c = 0;
    i = offset;
  } else {
    c = offset / COMPONENT_SIZE;
    i = offset % COMPONENT_SIZE;
  }
  if (c >= NCOMPONENTS || i >= data_[c].size()) {
    LOG(FATAL) << "IOVecBlock index out of range";
  }
  return data_[c].at(i);
}

bool IOVecBlock::safe_put(u8 val) {
  int c = pos_ / COMPONENT_SIZE;
  int i = pos_ % COMPONENT_SIZE;
  if (c >= NCOMPONENTS) {
    return false;
  }
  if (data_[c].empty()) {
    data_[c].resize(COMPONENT_SIZE);
  }
  data_[c][static_cast<size_t>(i)] = val;
  pos_++;
  return true;
}

int IOVecBlock::get_write_pos() const {
  return pos_;
}

void IOVecBlock::set_write_pos(int pos) {
  int c = pos / COMPONENT_SIZE;
  if (c >= NCOMPONENTS) {
    LOG(FATAL) << "Invalid shredded block write-position";
  }
  pos_ = pos;
}

void IOVecBlock::copy_from(const IOVecBlock& other) {
  // Single chunk
  if (other.data_[0].size() == FASTSTDB_BLOCK_SIZE) {
    u32 cons = 0;
    for (int i = 0; i < NCOMPONENTS; i++) {
      data_[i].resize(COMPONENT_SIZE);
      memcpy(data_[i].data(), other.data_[0].data() + cons, COMPONENT_SIZE);
      cons += COMPONENT_SIZE;
    }
  } else {
    for (int i = 0; i < NCOMPONENTS; i++) {
      if (other.data_[i].size() == 0) {
        return;
      }
      data_[i].resize(COMPONENT_SIZE);
      memcpy(data_[i].data(), other.data_[i].data(), COMPONENT_SIZE);
    }
  }
}

u32 IOVecBlock::read_chunk(void* dest, u32 offset, u32 size) {
  if (data_[0].size() == FASTSTDB_BLOCK_SIZE) {
    memcpy(dest, data_[0].data() + offset, size);
  } else {
    // Locate the component first
    u32 ixbegin  = offset / IOVecBlock::COMPONENT_SIZE;
    u32 offbegin = offset % IOVecBlock::COMPONENT_SIZE;
    u32 end      = offset + size;
    u32 ixend    = end / IOVecBlock::COMPONENT_SIZE;
    if (ixbegin == ixend) {
      // Fast path, access single component
      if (data_[ixbegin].size() == 0) {
        return 0;
      }
      u8* source = data_[ixbegin].data();
      memcpy(dest, source + offbegin, size);
    } else {
      // Read from two components
      if (data_[ixbegin].size() == 0) {
        return 0;
      }
      u8* c1 = data_[ixbegin].data();
      u32 l1  = IOVecBlock::COMPONENT_SIZE - offbegin;
      memcpy(dest, c1 + offbegin, l1);
      // Write second component
      if (data_[ixend].size() == 0) {
        return 0;
      }
      u32 l2 = size - l1;
      u8* c2 = data_[ixend].data();
      memcpy(static_cast<u8*>(dest) + l1, c2, l2);
    }
  }
  return size;
}

u32 IOVecBlock::append_chunk(const void* source, u32 size) {
  if (is_readonly()) {
    return 0;
  }
  if (data_[0].size() == FASTSTDB_BLOCK_SIZE) {
    // Fast path
    if (pos_ + size > FASTSTDB_BLOCK_SIZE) {
      return 0;
    }
    memcpy(data_[0].data() + pos_, source, size);
  } else {
    int ixbegin  = pos_ / IOVecBlock::COMPONENT_SIZE;
    int offbegin = pos_ % IOVecBlock::COMPONENT_SIZE;
    int end      = pos_ + size;
    int ixend    = end / IOVecBlock::COMPONENT_SIZE;
    if (ixbegin >= IOVecBlock::NCOMPONENTS || ixend >= IOVecBlock::NCOMPONENTS) {
      return 0;
    }
    if (ixbegin == ixend) {
      // Fast path, write to the single component
      if (data_[ixbegin].size() == 0) {
        int ixadd = add();
        if (ixadd != ixbegin) {
          LOG(FATAL) << "IOVec block corrupted";
        }
      }
      u8* dest = data_[ixbegin].data();
      memcpy(dest + offbegin, source, size);
    } else {
      // Write to two components
      if (data_[ixbegin].size() == 0) {
        int ixadd = add();
        if (ixadd != ixbegin) {
          LOG(FATAL) << "First IOVec block corrupted";
        }
      }
      u8* c1 = data_[ixbegin].data();
      u32 l1  = IOVecBlock::COMPONENT_SIZE - offbegin;
      memcpy(c1 + offbegin, source, l1);
      // Write second component
      if (data_[ixend].size() == 0) {
        int ixadd = add();
        if (ixadd != ixend) {
          LOG(FATAL) << "Second IOVec block corrupted";
        }
      }
      u32 l2 = size - l1;
      u8* c2 = data_[ixend].data();
      memcpy(c2, static_cast<const u8*>(source) + l1, l2);
    }
  }
  pos_ += size;
  return pos_;
}

void IOVecBlock::set_write_pos_and_shrink(int top) {
  set_write_pos(top);
  if (is_readonly() || data_[0].size() == FASTSTDB_BLOCK_SIZE) {
    return;
  }
  int component  = pos_ / IOVecBlock::COMPONENT_SIZE;
  for (int ix = IOVecBlock::NCOMPONENTS; ix --> 0;) {
    if (ix > component) {
      data_[ix].resize(0);
      data_[ix].shrink_to_fit();
    }
  }
}

const u8* IOVecBlock::get_data(int component) const {
  return data_[component].data();
}

const u8* IOVecBlock::get_cdata(int component) const {
  return data_[component].data();
}

u8* IOVecBlock::get_data(int component) {
  return data_[component].data();
}

size_t IOVecBlock::get_size(int component) const {
  return data_[component].size();
}

struct VolumeRef {
  u32 version;
  u32 id;
  u32 nblocks;
  u32 capacity;
  u32 generation;
  char path[];
};

static void volcpy(u8* block, const VolumeRegistry::VolumeDesc* desc) {
  VolumeRef* pvolume  = reinterpret_cast<VolumeRef*>(block);
  pvolume->capacity   = desc->capacity;
  pvolume->generation = desc->generation;
  pvolume->id         = desc->id;
  pvolume->nblocks    = desc->nblocks;
  pvolume->version    = desc->version;
  memcpy(pvolume->path, desc->path.data(), desc->path.size());
  pvolume->path[desc->path.size()] = '\0';
}

MetaVolume::MetaVolume(std::shared_ptr<VolumeRegistry> meta)
  : meta_(meta) {
  auto volumes = meta_->get_volumes();
  file_size_ = volumes.size() * FASTSTDB_BLOCK_SIZE;
  double_write_buffer_.resize(file_size_);
  std::set<u32> init_list;
  for (const auto& vol: volumes) {
    if (init_list.count(vol.id) != 0) {
      LOG(FATAL) << "Duplicate volumne record";
    }
    init_list.insert(vol.id);
    auto block = double_write_buffer_.data() + vol.id * FASTSTDB_BLOCK_SIZE;
    volcpy(block, &vol);
  }
}

size_t MetaVolume::get_nvolumes() const {
  return file_size_ / FASTSTDB_BLOCK_SIZE;
}

std::unique_ptr<MetaVolume> MetaVolume::open_existing(std::shared_ptr<VolumeRegistry> meta) {
  std::unique_ptr<MetaVolume> result;
  result.reset(new MetaVolume(meta));
  return result;
}

//! Helper function
static VolumeRef* get_volref(u8* p, u32 id) {
  u8* it = p + id * FASTSTDB_BLOCK_SIZE;
  VolumeRef* vol = reinterpret_cast<VolumeRef*>(it);
  return vol;
}

std::tuple<common::Status, u32> MetaVolume::get_nblocks(u32 id) const {
  if (id < file_size_ / FASTSTDB_BLOCK_SIZE) {
    auto pvol = get_volref(double_write_buffer_.data(), id);
    u32 nblocks = pvol->nblocks;
    return std::make_tuple(common::Status::Ok(), nblocks);
  }
  return std::make_tuple(common::Status::Internal("Id out of range"), 0u);
}

std::tuple<common::Status, u32> MetaVolume::get_capacity(u32 id) const {
  if (id < file_size_ / FASTSTDB_BLOCK_SIZE) {
    auto pvol = get_volref(double_write_buffer_.data(), id);
    u32 cap = pvol->capacity;
    return std::make_tuple(common::Status::Ok(), cap);
  }
  return std::make_tuple(common::Status::Internal("Id out of range"), 0u);
}

std::tuple<common::Status, u32> MetaVolume::get_generation(u32 id) const {
  if (id < file_size_ / FASTSTDB_BLOCK_SIZE) {
    auto pvol = get_volref(double_write_buffer_.data(), id);
    u32 gen = pvol->generation;
    return std::make_tuple(common::Status::Ok(), gen);
  }
  return std::make_tuple(common::Status::Internal("Id out of range"), 0u);
}

common::Status MetaVolume::add_volume(u32 id, u32 capacity, const std::string& path) {
  if (path.size() > FASTSTDB_BLOCK_SIZE - sizeof(VolumeRef)) {
    return common::Status::BadArg("");
  }

  size_t old_size = double_write_buffer_.size();
  double_write_buffer_.resize(old_size + FASTSTDB_BLOCK_SIZE);
  file_size_ += FASTSTDB_BLOCK_SIZE;
  u8* block = double_write_buffer_.data() + old_size;
  VolumeRef* pvolume  = reinterpret_cast<VolumeRef*>(block);
  pvolume->capacity   = capacity;
  pvolume->generation = id;
  pvolume->id         = id;
  pvolume->nblocks    = 0;
  pvolume->version    = FASTSTDB_VERSION;
  memcpy(pvolume->path, path.data(), path.size());
  pvolume->path[path.size()] = '\0';

  // Update metadata storage
  VolumeRegistry::VolumeDesc vol;
  vol.nblocks         = pvolume->nblocks;
  vol.generation      = pvolume->generation;
  vol.capacity        = pvolume->capacity;
  vol.version         = FASTSTDB_VERSION;
  vol.id              = pvolume->id;
  vol.path            = path;

  meta_->add_volume(vol);

  return common::Status::Ok();
}

common::Status MetaVolume::update(u32 id, u32 nblocks, u32 capacity, u32 gen) {
  if (id < file_size_ / FASTSTDB_BLOCK_SIZE) {
    auto pvol        = get_volref(double_write_buffer_.data(), id);
    pvol->nblocks    = nblocks;
    pvol->capacity   = capacity;
    pvol->generation = gen;

    // Update metadata storage (this update will be written into the sqlite
    // database eventually in the asynchronous manner.
    VolumeRegistry::VolumeDesc vol;
    vol.nblocks      = pvol->nblocks;
    vol.generation   = pvol->generation;
    vol.capacity     = pvol->capacity;
    vol.id           = pvol->id;
    vol.version      = pvol->version;
    vol.path.assign(static_cast<const char*>(pvol->path));
    meta_->update_volume(vol);

    return common::Status::Ok();
  }
  return common::Status::Internal("Id out of range");
}

common::Status MetaVolume::set_nblocks(u32 id, u32 nblocks) {
  if (id < file_size_ / FASTSTDB_BLOCK_SIZE) {
    auto pvol = get_volref(double_write_buffer_.data(), id);
    pvol->nblocks = nblocks;

    VolumeRegistry::VolumeDesc vol;
    vol.nblocks      = pvol->nblocks;
    vol.generation   = pvol->generation;
    vol.capacity     = pvol->capacity;
    vol.id           = pvol->id;
    vol.version      = pvol->version;
    vol.path.assign(static_cast<const char*>(pvol->path));
    meta_->update_volume(vol);

    return common::Status::Ok();
  }
  return common::Status::Internal("Id out of range");
}

common::Status MetaVolume::set_capacity(u32 id, u32 cap) {
  if (id < file_size_ / FASTSTDB_BLOCK_SIZE) {
    auto pvol = get_volref(double_write_buffer_.data(), id);
    pvol->capacity = cap;

    VolumeRegistry::VolumeDesc vol;
    vol.nblocks      = pvol->nblocks;
    vol.generation   = pvol->generation;
    vol.capacity     = pvol->capacity;
    vol.id           = pvol->id;
    vol.version      = pvol->version;
    vol.path.assign(static_cast<const char*>(pvol->path));
    meta_->update_volume(vol);

    return common::Status::Ok();
  }
  return common::Status::Internal("Id out of range");
}

common::Status MetaVolume::set_generation(u32 id, u32 gen) {
  if (id < file_size_ / FASTSTDB_BLOCK_SIZE) {
    auto pvol = get_volref(double_write_buffer_.data(), id);
    pvol->generation = gen;

    VolumeRegistry::VolumeDesc vol;
    vol.nblocks      = pvol->nblocks;
    vol.generation   = pvol->generation;
    vol.capacity     = pvol->capacity;
    vol.id           = pvol->id;
    vol.version      = pvol->version;
    vol.path.assign(static_cast<const char*>(pvol->path));
    meta_->update_volume(vol);

    return common::Status::Ok();
  }
  return common::Status::Internal("ID out of range");
}

void MetaVolume::flush() {
}

common::Status MetaVolume::flush(u32 id) {
  return common::Status::Ok();
}

static std::unique_ptr<common::MMapFile> GetMMapFile(const char* path) {
  std::unique_ptr<common::MMapFile> mmapFile(new common::MMapFile());
  auto status = mmapFile->Init(path);
  if (!status.IsOk()) {
    LOG(FATAL) << "GetMMapFile failed, path=" << path;
  }
  return mmapFile;
}

void CreateMMapFile(const char* path, u64 capacity) {
  std::unique_ptr<common::MMapFile> mmapFile(new common::MMapFile());
  mmapFile->Init(path, capacity * FASTSTDB_BLOCK_SIZE);
}

Volume::Volume(const char* path, size_t write_pos) :
    write_pos_(static_cast<u32>(write_pos)),
    path_(path),
    mmap_ptr_(nullptr) {
  mmap_ = GetMMapFile(path);
  mmap_ptr_ = reinterpret_cast<u8*>(mmap_->GetBase());
  file_size_ = mmap_->GetMmapSize() / FASTSTDB_BLOCK_SIZE;
}

void Volume::reset() {
  write_pos_ = 0;
}

void Volume::create_new(const char* path, u64 capacity) {
  CreateMMapFile(path, capacity);
}

std::unique_ptr<Volume> Volume::open_existing(const char* path, u64 pos) {
  std::unique_ptr<Volume> result;
  result.reset(new Volume(path, pos));
  return result;
}

//! Append block to file (source size should be 4 at least BLOCK_SIZE)
std::tuple<common::Status, BlockAddr> Volume::append_block(const u8* source) {
  if (write_pos_ >= file_size_) {
    return std::make_tuple(
        common::Status::Overflow(
            "write_pos_=" + std::to_string(write_pos_) +
            " file_size_=" + std::to_string(file_size_)), 0u);
  }

  auto offset = write_pos_ * FASTSTDB_BLOCK_SIZE;
  memcpy(mmap_ptr_ + offset, source, FASTSTDB_BLOCK_SIZE);
  auto result = write_pos_++;
  return std::make_tuple(common::Status::Ok(), result);
}

std::tuple<common::Status, BlockAddr> Volume::append_block(const IOVecBlock *source) {
  static std::vector<u8> padding(IOVecBlock::COMPONENT_SIZE);
  if (write_pos_ >= file_size_) {
    return std::make_tuple(
        common::Status::Overflow(
            "write_pos_=" + std::to_string(write_pos_) +
            " file_size_=" + std::to_string(file_size_)), 0u);
  }

  auto seek_off = write_pos_ * FASTSTDB_BLOCK_SIZE;
  auto fd = mmap_->fd();
  if (lseek(fd, seek_off, SEEK_SET) < 0) {
    return std::make_tuple(
        common::Status::FileSeekError(strerror(errno)), 0u);
  }
  
  struct iovec vec[IOVecBlock::NCOMPONENTS] = {};
  size_t nvec = 0;
  for (int i = 0; i < IOVecBlock::NCOMPONENTS; i++) {
    if (source->get_size(i) != 0) {
      vec[i].iov_base = const_cast<u8*>(source->get_data(i));
      vec[i].iov_len  = IOVecBlock::COMPONENT_SIZE;
    } else {
      vec[i].iov_base = const_cast<u8*>(padding.data());
      vec[i].iov_len  = IOVecBlock::COMPONENT_SIZE;
    }
    nvec++;
  }

  auto size = writev(fd, vec, nvec);
  if (size != FASTSTDB_BLOCK_SIZE) {
    return std::make_tuple(common::Status::FileWriteError(strerror(errno)), 0u);
  }
  auto result = write_pos_++;
  return std::make_tuple(common::Status::Ok(), result);
}

//! Read filxed size block from file
common::Status Volume::read_block(u32 ix, u8* dest) const {
  if (ix >= write_pos_) {
    return common::Status::Overflow(
        "ix=" + std::to_string(ix) +
        " write_pos_=" + std::to_string(write_pos_));
  }
  u64 offset = ix * FASTSTDB_BLOCK_SIZE;
  memcpy(dest, mmap_ptr_ + offset, FASTSTDB_BLOCK_SIZE);
  return common::Status::Ok();
}

std::tuple<common::Status, std::unique_ptr<IOVecBlock>> Volume::read_block(u32 ix) const {
  std::unique_ptr<IOVecBlock> block;
  block.reset(new IOVecBlock(true));
  
  u8* data = block->get_data(0);
  u32 size = block->get_size(0);
  if (size != FASTSTDB_BLOCK_SIZE) {
    return std::make_tuple(common::Status::BadArg(""), std::move(block));
  }
  auto status = read_block(ix, data);
  return std::make_tuple(status, std::move(block));
}

std::tuple<common::Status, const u8*> Volume::read_block_zero_copy(u32 ix) const {
  if (ix >= write_pos_) {
    return std::make_tuple(
        common::Status::Overflow(
            "ix=" + std::to_string(ix) +
            " write_pos_=" + std::to_string(write_pos_)), nullptr);
  }
  u64 offset = ix * FASTSTDB_BLOCK_SIZE;
  auto ptr = mmap_ptr_ + offset;
  return std::make_tuple(common::Status::Ok(), ptr);
}

void Volume::flush() {
}

u32 Volume::get_size() const {
  return file_size_;
}

std::string Volume::get_path() const {
  return path_;
}

}  // namespace storage
}  // namespace faststdb
