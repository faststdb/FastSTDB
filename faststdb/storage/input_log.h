/*!
 * \file input_log.h
 */
#ifndef FASTSTDB_STORAGE_INPUT_LOG_H_
#define FASTSTDB_STORAGE_INPUT_LOG_H_

#include <string>
#include <vector>
#include <memory>
#include <deque>
#include <atomic>

#include <apr.h>
#include <apr_file_io.h>
#include <apr_general.h>

#include <boost/filesystem.hpp>
#include <boost/variant.hpp>

#include "lz4.h"
#include "faststdb/common/basic.h"
#include "faststdb/common/logging.h"
#include "faststdb/common/status.h"

namespace roaring {
// Fwd declaration
class Roaring64Map;
}  // namespace roaring

namespace faststdb {
namespace storage {

typedef std::unique_ptr<apr_pool_t, void (*)(apr_pool_t*)> AprPoolPtr;
typedef std::unique_ptr<apr_file_t, void (*)(apr_file_t*)> AprFilePtr;

struct InputLogDataPoint {
  u64 timestamp;
  double value;
};

struct InputLogSeriesName {
  std::string value;
};

struct InputLogRecoveryInfo {
  std::vector<u64> data;
};

struct InputLogRow {
  boost::variant<InputLogDataPoint,
      InputLogSeriesName,
      InputLogRecoveryInfo> payload;
  u64         id;
};

/**
  Sequencing.

  Each log record should have a sequence number.

  All log records, generated by different threads can be ordered during the recovery
  stage using these sequence numbers. The problem that I'm trying to solve here is that
  we might have many threads that generate log records. The client might reconnect
  several times being dispatched to different threads thus, different volumes might
  have the data from the same series. If we'll just merge everything in naive way,
  the values might get scrambled and some writes might fail ruining the recovery process.
  To fight this we need a way to order the frames from various sources. Every frame
  have a sequence number that gets incremented each time we write the frame.
  To make the sequencing correct we should force the current frame to be writtent to disk
  when the client disconnects.
 */

/**
 * Unique number generator for the input log.
 * TBD
 */
struct LogSequencer {
  std::atomic<u64> counter_;

  LogSequencer();
  ~LogSequencer() = default;

  u64 next();
};

/** LZ4 compressed volume for single-threaded use.
*/
struct LZ4Volume {
  std::string path_;

  enum class FrameType : u8 {
    EMPTY = 0,
    DATA_ENTRY = 1,
    SNAME_ENTRY = 2,
    RECOVERY_ENTRY = 4,
  };

  struct FrameHeader {
    FrameType frame_type;
    u16 magic;
    u64 sequence_number;
    u32 size;
  };

  enum {
    BLOCK_SIZE          = 0x2000,
    FRAME_TUPLE_SIZE    = sizeof(u64)*3,
    NUM_TUPLES          = (BLOCK_SIZE - sizeof(FrameHeader)) / FRAME_TUPLE_SIZE,
  };

  union Frame {
    char block[BLOCK_SIZE];
    FrameHeader header;
    struct DataEntry : FrameHeader {
      u64 ids[NUM_TUPLES];
      u64 tss[NUM_TUPLES];
      double xss[NUM_TUPLES];
    } data_points;
    // This structure is used to implement storage for series names
    // and recovery arrays.
    struct FlexibleEntry : FrameHeader {
      char data[BLOCK_SIZE - sizeof(FrameHeader)];
      u64 vector[0];
    } payload;
  } frames_[2];

  static_assert(sizeof(Frame) == BLOCK_SIZE, "Frame is missaligned");
  static_assert(sizeof(Frame::DataEntry) <= BLOCK_SIZE, "Frame::DataEntry is missaligned");
  static_assert(BLOCK_SIZE - sizeof(Frame::DataEntry) < FRAME_TUPLE_SIZE, "Frame::DataEntry is too small");
  static_assert(sizeof(Frame::FlexibleEntry) == BLOCK_SIZE, "Frame::FlexibleEntry is missaligned");
  static_assert(NUM_TUPLES*(sizeof(u64) + sizeof(u64) + sizeof(double)) < BLOCK_SIZE - sizeof(FrameHeader), "DataEntry is too big");

  char buffer_[LZ4_COMPRESSBOUND(BLOCK_SIZE)];

  int pos_;
  LZ4_stream_t stream_;
  LZ4_streamDecode_t decode_stream_;
  AprPoolPtr pool_;
  AprFilePtr file_;
  size_t file_size_;
  const size_t max_file_size_;
  std::shared_ptr<roaring::Roaring64Map> bitmap_;
  const bool is_read_only_;
  i64 bytes_to_read_;
  int elements_to_read_;  // in current frame
  LogSequencer *sequencer_;

  void clear(int i);

  common::Status write(int i);

  std::tuple<common::Status, size_t> read(int i);

  /** Check if the current frame is of required type.
   * If this is the case the method will do nothing
   * and return SUCCESS. If the current frame has
   * different type the method will flush it. If the
   * frame is empty it will be initialized for the
   * specific type.
   */
  common::Status require_frame_type(FrameType type);

  /** Writes current frame to log. Set type of the
   * next frame to 'type'.
   */
  common::Status flush_current_frame(FrameType type);

  /** Implementation for recovery and sname frames */
  common::Status append_blob(FrameType type, u64 id, const char* payload, u32 len);

 public:
  /**
   * @brief Create empty volume
   * @param file_name is string that contains volume file name
   * @param volume_size is a maximum allowed volume size
   */
  LZ4Volume(LogSequencer* sequencer, const char* file_name, size_t volume_size);

  /**
   * @brief Create volume for existing log file.
   * @param file_name volume file name
   * @note `open_ro` should be called before reading.
   */
  LZ4Volume(const char* file_name);

  ~LZ4Volume();

  //! Open file in read-only mode
  void open_ro();

  bool is_opened() const;

  void close();

  size_t file_size() const;

  common::Status append(u64 id, u64 timestamp, double value);
  common::Status append(u64 id, const char* sname, u32 len);
  common::Status append(u64 id, const u64* recovery_array, u32 len);

  /**
   * @brief Read values in bulk (volume should be opened in read mode)
   * @param buffer_size is a size of any input buffer (all should be of the same size)
   * @param id is a pointer to buffer that should receive up to `buffer_size` ids
   * @param ts is a pointer to buffer that should receive `buffer_size` timestamps
   * @param xs is a pointer to buffer that should receive `buffer_size` values
   * @return number of elements being read or 0 if EOF reached or negative value on error
   */
  std::tuple<common::Status, u32> read_next(size_t buffer_size, u64* id, u64* ts, double* xs);
  std::tuple<common::Status, u32> read_next(size_t buffer_size, InputLogRow* rows);

  /**
   * @brief Read next frame from the volume
   * @return status and pointer to frame
   */
  std::tuple<common::Status, const Frame*> read_next_frame();

  const std::string get_path() const;

  void delete_file();

  const roaring::Roaring64Map& get_index() const;

  //! Flush current frame to disk.
  common::Status flush();
};

class InputLog {
  typedef boost::filesystem::path Path;
  std::deque<std::unique_ptr<LZ4Volume>> volumes_;
  Path root_dir_;
  size_t volume_counter_;
  const size_t max_volumes_;
  const size_t volume_size_;
  std::vector<Path> available_volumes_;
  const u32 stream_id_;
  LogSequencer* sequencer_;

  void find_volumes();

  void open_volumes();

  std::string get_volume_name();

  void add_volume(std::string path);

  void remove_last_volume();

  void detect_stale_ids(std::vector<u64> *stale_ids);
 
 public:
  /**
   * @brief Create writeable input log
   * @param rootdir is a directory containing all volumes
   * @param nvol max number of volumes
   * @param svol individual volume size
   * @param id is a stream id (for sharding)
   * @param sequencer is a pointer to log sequencer used to generate seq-numbers
   */
  InputLog(LogSequencer* sequencer, const char* rootdir, size_t nvol, size_t svol, u32 stream_id);

  /**
   * @brief Recover information from input log
   * @param rootdir is a directory containing all volumes
   */
  InputLog(const char* rootdir, u32 stream_id);

  void reopen();

  /** Delete all files.
  */
  void delete_files();

  /** Append data point to the log.
   * Return true on oveflow. Parameter `stale_ids` will be filled with ids that will leave the
   * input log on next rotation. Rotation should be triggered manually.
   */
  common::Status append(u64 id, u64 timestamp, double value, std::vector<u64>* stale_ids);
  common::Status append(u64 id, const char* sname, u32 len, std::vector<u64> *stale_ids);
  common::Status append(u64 id, const u64* rescue_points, u32 len, std::vector<u64> *stale_ids);

  /**
   * @brief Read values in bulk (volume should be opened in read mode)
   * @param buffer_size is a size of any input buffer (all should be of the same size)
   * @param id is a pointer to buffer that should receive up to `buffer_size` ids
   * @param ts is a pointer to buffer that should receive `buffer_size` timestamps
   * @param xs is a pointer to buffer that should receive `buffer_size` values
   * @return number of elements being read or 0 if EOF reached or negative value on error
   */
  std::tuple<common::Status, u32> read_next(size_t buffer_size, u64* id, u64* ts, double* xs);
  std::tuple<common::Status, u32> read_next(size_t buffer_size, InputLogRow* rows);

  /**
   * @brief Read next frame from the volume
   * @return status and pointer to frame
   */
  std::tuple<common::Status, const LZ4Volume::Frame*> read_next_frame();

  void rotate();

  /** Write current frame to disk if it has any data.
  */
  common::Status flush(std::vector<u64>* stale_ids);
};

/** Wrapper for input log that implements microsharding.
 * Each worker thread should have it's own InputLog instance.
 * During recovery, the component should read data from all
 * shards in parallel and merge it based on timestamp and id.
 * This is needed for the case when client that sends particular
 * metric reconnects and gets handled by the other worker thread.
 */
class ShardedInputLog {
  std::vector<std::unique_ptr<InputLog>> streams_;
  int concurrency_;
  LogSequencer sequencer_;

  enum {
    NUM_TUPLES = LZ4Volume::NUM_TUPLES,
  };
  typedef LZ4Volume::Frame Frame;

  struct Buffer {
    u32          pos;
    common::Status   status;
    const Frame* frame;
  };
  std::vector<Buffer> read_queue_;
  bool read_only_;       //! Will be set to true if the log was opened in read-only mode
  bool read_started_;    //! Will be set to true if the read operation is in progress
  int buffer_ix_;        //! Read position
  std::string rootdir_;  //! Root-dir for reopen method
  size_t nvol_;          //! Number of volumes to create in write-only mode
  size_t svol_;          //! Size of the volume in write-only mode

  void init_read_buffers();

  //! Select next buffer with smallest sequence number
  int choose_next();

  //! Refill used buffer
  void refill_buffer(int ix);

 public:
  /**
   * @brief Find log files in `rootdir`
   * @param rootdir is a path to directory with log files
   * @return 0 if no log files found, concurrency level otherwise
   */
  static std::tuple<common::Status, int> find_logs(const char* rootdir);

  /**
   * @brief Create ShardedInputLog that can be used to write data
   * @param concurrency is a concurrency level of the logger
   * @param rootdir is a root directory of the logger
   * @param nvol is a limit on number of volumes (per thread)
   * @param svol is a limit on a size of the individual volume
   */
  ShardedInputLog(int concurrency, const char* rootdir, size_t nvol, size_t svol);

  /**
   * @brief Create SharedInputLog that can be used to recover the data
   * @param concurrency is a concurrency level of the logger
   * @param rootdir is a filesystem path that will be used to search for a data
   */
  ShardedInputLog(int concurrency, const char* rootdir);

  InputLog& get_shard(int i);

  /**
   * @brief Read values in bulk (volume should be opened in read mode)
   * @param buffer_size is a size of any input buffer (all should be of the same size)
   * @param id is a pointer to buffer that should receive up to `buffer_size` ids
   * @param ts is a pointer to buffer that should receive `buffer_size` timestamps
   * @param xs is a pointer to buffer that should receive `buffer_size` values
   * @return number of elements being read or 0 if EOF reached
   */
  std::tuple<common::Status, u32> read_next(size_t buffer_size, u64* id, u64* ts, double* xs);
  std::tuple<common::Status, u32> read_next(size_t buffer_size, InputLogRow* rows);

  /**
   * Reopen log if it was opened in read-only mode. This allows to read content once
   * again or delete files.
   */
  void reopen();

  /**
   * Delete all log files.
   * If the data was retreived no files will be deleted.
   */
  void delete_files();
};

}  // namespace storage
}  // namespace faststdb

#endif  // FASTSTDB_STORAGE_INPUT_LOG_H_
