/**
 * \file storage.h
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#ifndef FASTSTDB_CORE_STORAGE_H_
#define FASTSTDB_CORE_STORAGE_H_

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <stack>
#include <thread>
#include <vector>
#include <future>

#include "faststdb/core/metadatastorage.h"
#include "faststdb/index/seriesparser.h"
#include "faststdb/query/queryprocessor_framework.h"

#include "faststdb/storage/block_store.h"
#include "faststdb/storage/nbtree.h"
#include "faststdb/storage/column_store.h"
#include "faststdb/storage/input_log.h"

#include "faststdb/query/internal_cursor.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/thread.hpp>
#include <boost/thread/tss.hpp>

namespace faststdb {

class Storage;

class StorageSession : public std::enable_shared_from_this<StorageSession> {
  std::shared_ptr<Storage> storage_;
  PlainSeriesMatcher local_matcher_;
  std::shared_ptr<storage::CStoreSession> session_;
  //! Temporary query matcher
  mutable std::shared_ptr<PlainSeriesMatcher> matcher_substitute_;
  storage::ShardedInputLog* slog_;
  storage::InputLog* ilog_;

 public:
  StorageSession(std::shared_ptr<Storage> storage,
                 std::shared_ptr<storage::CStoreSession> session,
                 storage::ShardedInputLog* log);

  ~StorageSession();

  common::Status write(Sample const& sample);

  /** Match series name. If series with such name doesn't exists - create it.
   * This method should be called for each sample to init its `paramid` field.
   */
  common::Status init_series_id(const char* begin, const char* end, Sample *sample);

  /** Match series name in joined form (foo:bar:buz tag=val will is a shorthand for foo tag=val, bar tag=val
   * and buz tag=val). Return array of series ids in order (foo, bar, buz) through ids array.
   * Return number of series or negative value on error (error code * -1).
   */
  int get_series_ids(const char* begin, const char* end, ParamId* ids, size_t ids_size);

  int get_series_name(ParamId id, char* buffer, size_t buffer_size);

  void query(InternalCursor* cur, const char* query) const;

  /**
   * @brief suggest query implementation
   * @param cur is a pointer to internal cursor
   * @param query is a string that contains query
   */
  void suggest(InternalCursor* cur, const char* query) const;

  /**
   * @brief search query implementation
   * @param cur is a pointer to internal cursor
   * @param query is a string that contains query
   */
  void search(InternalCursor* cur, const char* query) const;

  // Temporary reset series matcher
  void set_series_matcher(std::shared_ptr<PlainSeriesMatcher> matcher) const;
  void clear_series_matcher() const;
};

class Storage : public std::enable_shared_from_this<Storage> {
  std::shared_ptr<storage::BlockStore> bstore_;
  std::shared_ptr<storage::ColumnStore> cstore_;
  std::atomic<int> done_;
  boost::barrier close_barrier_;
  mutable std::mutex lock_;
  SeriesMatcher global_matcher_;
  std::shared_ptr<MetadataStorage> metadata_;
  std::shared_ptr<storage::ShardedInputLog> inputlog_;
  std::string input_log_path_;

  // Await support
  std::vector<std::promise<void>> sessions_await_list_;
  std::mutex session_lock_;

  void start_sync_worker();

  std::tuple<common::Status, std::string> parse_query(const boost::property_tree::ptree &ptree,
                                                      qp::ReshapeRequest* req) const;

  void run_inputlog_recovery(storage::ShardedInputLog* ilog, std::vector<ParamId> ids2restore);

  void run_inputlog_metadata_recovery(storage::ShardedInputLog* ilog, std::vector<ParamId> *restored_ids,
                                      std::unordered_map<ParamId, std::vector<storage::LogicAddr>>* mapping);
 public:
  // Create empty in-memory storage
  Storage();

  /**
   * @brief Open storage engine
   * @param path is a path to main files
   */
  Storage(const char*                                 path,
          const FineTuneParams&                   params);

  /** C-tor for test */
  Storage(std::shared_ptr<MetadataStorage>            meta,
          std::shared_ptr<storage::BlockStore>  bstore,
          std::shared_ptr<storage::ColumnStore> cstore,
          bool                                        start_worker);

  void run_recovery(const FineTuneParams &params,
                    std::unordered_map<ParamId, std::vector<storage::LogicAddr>>* mapping);

  //! Perform input log recovery if needed and initialize input log
  void initialize_input_log(const FineTuneParams& params);

  //! Match series name. If series with such name doesn't exists - create it.
  std::tuple<common::Status, bool> init_series_id(const char* begin, const char* end, Sample *sample, PlainSeriesMatcher *local_matcher);

  int get_series_name(ParamId id, char* buffer, size_t buffer_size, PlainSeriesMatcher *local_matcher);

  //! Create new write session
  std::shared_ptr<StorageSession> create_write_session();

  void query(StorageSession const* session, InternalCursor* cur, const char* query) const;

  /**
   * @brief suggest query implementation
   * @param session is a session pointer
   * @param cur is an internal cursor
   * @param query is a query string (JSON)
   */
  void suggest(StorageSession const* session, InternalCursor* cur, const char* query) const;

  /**
   * @brief search query implementation
   * @param session is a session pointer
   * @param cur is an internal cursor
   * @param query is a query string (JSON)
   */
  void search(StorageSession const* session, InternalCursor* cur, const char* query) const;

  void debug_print() const;

  void _update_rescue_points(ParamId id, std::vector<storage::LogicAddr>&& rpoints);

  /** This method should be called before object destructor.
   * All ingestion sessions should be stopped first.
   */
  void close();

  /**
   * @brief Flush and close every column in the list
   * @param ids list of column ids
   * @return status
   */
  void close_specific_columns(const std::vector<u64>& ids);

  /** Create empty database from scratch.
   * @param base_file_name is database name (excl suffix)
   * @param metadata_path is a path to metadata storage
   * @param volumes_path is a path to volumes storage
   * @param num_volumes defines how many volumes should be crated
   * @param page_size is a size of the individual page in bytes
   * @return operation status
   */
  static common::Status new_database( const char     *base_file_name
                                     , const char     *metadata_path
                                     , const char     *volumes_path
                                     , i32             num_volumes
                                     , u64             page_size
                                     , bool            allocate);

  /**
   * @brief Open storage and generate report (dont' modify anything)
   * @param path to sqlite3 database file
   * @return status
   */
  static common::Status generate_report(const char* path, const char* output);

  static common::Status generate_recovery_report(const char* path, const char* output);

  /** Remove existing database
   * @param file_name is a database name
   * @param force forces database deletion even if database is not empty
   * @return SUCCESS on success or ENOT_PERMITTED if database contains data and `force` is false or
   *         EACCESS if database there is not enough priveleges to delete the files
   */
  static common::Status remove_storage(const char* file_name, const char *wal_path, bool force);

  boost::property_tree::ptree get_stats();

  /** Destroy object without preserving consistency
  */
  void _kill();

  void add_metadata_sync_barrier(std::promise<void>&& barrier);
};

}  // namespace faststdb

#endif  // FASTSTDB_CORE_STORAGE_H_
