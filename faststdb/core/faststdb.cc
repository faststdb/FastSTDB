/**
 * \file faststdb.cc
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include "faststdb/core/faststdb.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <memory>
#include <iostream>

#include <apr_dbd.h>

#include "faststdb/common/datetime.h"
#include "faststdb/common/thread_local.h"
#include "faststdb/core/cursor.h"
#include "faststdb/core/storage.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace faststdb;

typedef common::ThreadLocalStore<std::string> ThreadLocalMsg;

//! Pool for `apr_dbd_init`
static apr_pool_t* g_dbd_pool = nullptr;

void initialize() {
  // initialize libapr
  apr_initialize();
  // initialize aprdbd
  auto status = apr_pool_create(&g_dbd_pool, nullptr);
  if (status != APR_SUCCESS) {
    LOG(FATAL) << "Initialization error";
  }
  status = apr_dbd_init(g_dbd_pool);
  if (status != APR_SUCCESS) {
    LOG(FATAL) << "DBD initialization error";
  }
}

int debug_report_dump(const char* path2db, const char* outfile) {
  return Storage::generate_report(path2db, outfile).Code();
}

int debug_recovery_report_dump(const char* path2db, const char* outfile) {
  return Storage::generate_recovery_report(path2db, outfile).Code();
}

const char* error_message(int error_code) {
  *(ThreadLocalMsg::Get()) = common::Status((common::Status::ErrorCode)error_code).ToString();
  return ThreadLocalMsg::Get()->c_str();
}

struct CursorImpl : Cursor {
  std::unique_ptr<ExternalCursor> cursor_;
  std::string query_;

  CursorImpl(std::shared_ptr<StorageSession> storage, const char* query)
      : query_(query)
  {
    cursor_ = ConcurrentCursor::make(&StorageSession::query, storage, query_.data());
  }

  ~CursorImpl() {
    cursor_->close();
  }

  bool is_done() const {
    return cursor_->is_done();
  }

  bool is_error(common::Status* out_error_code_or_null) const {
    return cursor_->is_error(out_error_code_or_null);
  }

  bool is_error(const char** error_msg, common::Status* out_error_code_or_null) const {
    return cursor_->is_error(error_msg, out_error_code_or_null);
  }

  u32 read_values( void  *values
                  , u32    values_size )
  {
    return cursor_->read(values, values_size);
  }
};

/**
 * Cursor that returns results of the 'suggest' query
 * used by Grafana.
 */
struct SuggestCursorImpl : Cursor {
  std::unique_ptr<ExternalCursor> cursor_;
  std::string query_;

  SuggestCursorImpl(std::shared_ptr<StorageSession> storage, const char* query)
      : query_(query)
  {
    cursor_ = ConcurrentCursor::make(&StorageSession::suggest, storage, query_.data());
  }

  ~SuggestCursorImpl() {
    cursor_->close();
  }

  bool is_done() const {
    return cursor_->is_done();
  }

  bool is_error(common::Status* out_error_code_or_null) const {
    return cursor_->is_error(out_error_code_or_null);
  }

  bool is_error(const char** error_message, common::Status* out_error_code_or_null) const {
    return cursor_->is_error(error_message, out_error_code_or_null);
  }

  u32 read_values( void  *values
                  , u32    values_size )
  {
    return cursor_->read(values, values_size);
  }
};

/**
 * Cursor that returns results of the 'search' query.
 */
struct SearchCursorImpl : Cursor {
  std::unique_ptr<ExternalCursor> cursor_;
  std::string query_;

  SearchCursorImpl(std::shared_ptr<StorageSession> storage, const char* query)
      : query_(query)
  {
    cursor_ = ConcurrentCursor::make(&StorageSession::search, storage, query_.data());
  }

  ~SearchCursorImpl() {
    cursor_->close();
  }

  bool is_done() const {
    return cursor_->is_done();
  }

  bool is_error(common::Status* out_error_code_or_null) const {
    return cursor_->is_error(out_error_code_or_null);
  }

  bool is_error(const char** error_message, common::Status* out_error_code_or_null) const {
    return cursor_->is_error(error_message, out_error_code_or_null);
  }

  u32 read_values( void  *values
                  , u32    values_size )
  {
    return cursor_->read(values, values_size);
  }
};

class FSession : public Session {
  std::shared_ptr<StorageSession> session_;
 
 public:
  FSession(std::shared_ptr<StorageSession> session)
      : session_(session) { }

  common::Status series_to_param_id(const char* begin, const char* end, Sample *out_sample) {
    return session_->init_series_id(begin, end, out_sample);
  }

  int name_to_param_id_list(const char* begin, const char* end, ParamId* out_ids, u32 out_ids_cap) {
    return session_->get_series_ids(begin, end, out_ids, out_ids_cap);
  }

  int param_id_to_series(ParamId id, char* buffer, size_t size) const {
    return session_->get_series_name(id, buffer, size);
  }

  common::Status add_sample(Sample const& sample) {
    return session_->write(sample);
  }

  CursorImpl* query(const char* q) {
    auto res = new CursorImpl(session_, q);
    return res;
  }

  SuggestCursorImpl* suggest(const char* q) {
    auto res = new SuggestCursorImpl(session_, q);
    return res;
  }

  SearchCursorImpl* search(const char* q) {
    auto res = new SearchCursorImpl(session_, q);
    return res;
  }
};

/** 
 * Object that extends a Database struct.
 * Can be used from "C" code.
 */
class DatabaseImpl : public Database {
  std::shared_ptr<Storage> storage_;

 public:
  // private fields
  DatabaseImpl(const char* path, const FineTuneParams& params) {
    if (path == std::string(":memory:")) {
      storage_ = std::make_shared<Storage>();
    } else {
      storage_ = std::make_shared<Storage>(path, params);
      storage_->initialize_input_log(params);
    }
  }

  void close() {
    storage_->close();
  }

  static Database* create(const char* path, const FineTuneParams& params) {
    DatabaseImpl* ptr = new DatabaseImpl(path, params);
    return static_cast<Database*>(ptr);
  }

  static void free(Database* ptr) {
    DatabaseImpl* pimpl = reinterpret_cast<DatabaseImpl*>(ptr);
    pimpl->close();
    delete pimpl;
  }

  static void free(Session* ptr) {
    auto pimpl = reinterpret_cast<FSession*>(ptr);
    delete pimpl;
  }

  void debug_print() const {
    storage_->debug_print();
  }

  Session* create_session() {
    auto disp = storage_->create_write_session();
    FSession* ptr = new FSession(disp);
    return static_cast<Session*>(ptr);
  }

  boost::property_tree::ptree get_stats() {
    return storage_->get_stats();
  }
};

int create_database_ex(const char     *base_file_name,
                       const char     *metadata_path,
                       const char     *volumes_path,
                       i32             num_volumes,
                       u64             page_size,
                       bool            allocate) {
  auto status = Storage::new_database(base_file_name, metadata_path, volumes_path, num_volumes, page_size, allocate);
  return status.Code();
}

int create_database(const char     *base_file_name,
                        const char     *metadata_path,
                        const char     *volumes_path,
                        i32             num_volumes,
                        bool            allocate) {
  static const u64 vol_size = 4096ull * 1024 * 1024; // pages (4GB total)
  return create_database_ex(base_file_name, metadata_path, volumes_path, num_volumes, vol_size, allocate);
}

Database* open_database(const char* path, FineTuneParams parameters) {
  return DatabaseImpl::create(path, parameters);
}

void close_database(Database* db) {
  DatabaseImpl::free(db);
}

int remove_database(const char* file_name, const char* wal_path, bool force) {
  auto status = Storage::remove_storage(file_name, wal_path, force);
  return status.Code();
}

Session* create_session(Database* db) {
  auto dbi = reinterpret_cast<DatabaseImpl*>(db);
  return dbi->create_session();
}

void destroy_session(Session* session) {
  DatabaseImpl::free(session);
}

int write_double_raw_sample(Session* session, ParamId param_id, Timestamp timestamp,  double value) {
  Sample sample;
  sample.timestamp = timestamp;
  sample.paramid = param_id;
  sample.payload.type = PAYLOAD_FLOAT;
  sample.payload.float64 = value;
  auto ises = reinterpret_cast<FSession*>(session);
  auto status = ises->add_sample(sample);
  return status.Code();
}

int write_sample(Session* session, const Sample* sample) {
  auto ises = reinterpret_cast<FSession*>(session);
  auto status = ises->add_sample(*sample);
  return status.Code();
}

int parse_duration(const char* str, int* value) {
  try {
    *value = DateTimeUtil::parse_duration(str, strlen(str));
  } catch (...) {
    return common::Status::kBadArg;
  }
  return common::Status::kOk;
}

int parse_timestamp(const char* iso_str, Sample* sample) {
  try {
    sample->timestamp = DateTimeUtil::from_iso_string(iso_str);
  } catch (...) {
    return common::Status::kBadArg;
  }
  return common::Status::kOk;
}

int series_to_param_id(Session* session, const char* begin, const char* end, Sample* sample) {
  auto ises = reinterpret_cast<FSession*>(session);
  auto status = ises->series_to_param_id(begin, end, sample);
  return status.Code();
}

int name_to_param_id_list(Session* ist, const char* begin, const char* end, ParamId* out_ids, u32 out_ids_cap) {
  auto ises = reinterpret_cast<FSession*>(ist);
  return ises->name_to_param_id_list(begin, end, out_ids, out_ids_cap);
}

Cursor* query(Session* session, const char* query) {
  auto impl = reinterpret_cast<FSession*>(session);
  auto cursor = impl->query(query);
  return static_cast<Cursor*>(cursor);
}

Cursor* suggest(Session* session, const char* query) {
  auto impl = reinterpret_cast<FSession*>(session);
  auto cursor = impl->suggest(query);
  return static_cast<Cursor*>(cursor);
}

Cursor* search(Session* session, const char* query) {
  auto impl = reinterpret_cast<FSession*>(session);
  auto cursor = impl->search(query);
  return static_cast<Cursor*>(cursor);
}

void cursor_close(Cursor* pcursor) {
  auto impl = reinterpret_cast<CursorImpl*>(pcursor);
  delete impl;  // destructor calls `close` method
}

size_t cursor_read(Cursor       *cursor,
                   void             *dest,
                   size_t            dest_size) {
  auto impl = reinterpret_cast<CursorImpl*>(cursor);
  return impl->read_values(dest, static_cast<u32>(dest_size));
}

int cursor_is_done(Cursor* pcursor) {
  auto impl = reinterpret_cast<CursorImpl*>(pcursor);
  return impl->is_done();
}

int cursor_is_error(Cursor* pcursor) {
  auto impl = reinterpret_cast<CursorImpl*>(pcursor);
  common::Status out_error_code_or_null;
  return impl->is_error(&out_error_code_or_null);
}

int cursor_is_error_ex(Cursor* pcursor) {
  auto impl = reinterpret_cast<CursorImpl*>(pcursor);
  const char* error_message;
  common::Status out_error_code_or_null;
  return impl->is_error(&error_message, &out_error_code_or_null);
}

int timestamp_to_string(Timestamp ts, char* buffer, size_t buffer_size) {
  return DateTimeUtil::to_iso_string(ts, buffer, buffer_size);
}

int param_id_to_series(Session* session, ParamId id, char* buffer, size_t buffer_size) {
  auto ises = reinterpret_cast<FSession*>(session);
  return ises->param_id_to_series(id, buffer, buffer_size);
}

//--------------------------------
//         Statistics
//--------------------------------
void global_search_stats(SearchStats* rcv_stats, int reset) {
  LOG(FATAL) << "Not implemented";
}

void global_storage_stats(Database *db, StorageStats* rcv_stats) {
  LOG(FATAL) << "Not implemented";
}

int json_stats(Database *db, char* buffer, size_t size) {
  auto dbi = reinterpret_cast<DatabaseImpl*>(db);
  try {
    auto ptree = dbi->get_stats();
    // encode json
    std::stringstream out;
    boost::property_tree::json_parser::write_json(out, ptree, true);
    auto str = out.str();
    if (str.size() > size) {
      return -1*static_cast<int>(str.size());
    }
    strcpy(buffer, str.c_str());
    return static_cast<int>(str.size());
  } catch (std::exception const& e) {
    LOG(ERROR) << e.what();
  } catch (...) {
    LOG(FATAL) << "unexpected error in `json_stats`";
  }
  return -1;
}

void debug_print(Database *db) {
  LOG(FATAL) << "Not implemented";
}

int get_resource(const char* res_name, char* buf, size_t* bufsize) {
  static const std::set<std::string> RESOURCES = {
    "function-names",
    "version"
  };
  std::string res(res_name);
  if (RESOURCES.count(res) == 0) {
    return common::Status::kBadArg;
  }
  if (res == "function-names") {
    auto names = qp::list_query_registry();
    std::string result;
    for (auto name: names) {
      result += name;
      result += "\n";
    }
    if (result.size() > *bufsize) {
      return common::Status::kOverflow;
    }
    std::copy(result.begin(), result.end(), buf);
    *bufsize = result.size();
  } else if (res == "version") {
    std::string result = "FastSTDB " + std::to_string(FASTSTDB_VERSION);  // add build info
    if (result.size() > *bufsize) {
      return common::Status::kOverflow;
    }
    std::copy(result.begin(), result.end(), buf);
    *bufsize = result.size();
  }
  return common::Status::kOk;
}
