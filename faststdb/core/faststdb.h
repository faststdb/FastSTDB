/** 
 * \file faststdb.h
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
 */
#ifndef FASTSTDB_FASTSTDB_H_
#define FASTSTDB_FASTSTDB_H_

#include <apr_errno.h>
#include <stdint.h>

#include "faststdb/common/config.h"

#if !defined _WIN32
#ifdef __cplusplus
#define EXPORT extern "C" __attribute__((visibility("default")))
#else
#define EXPORT
#endif
#else
#define EXPORT __declspec(dllexport)
#endif

//! Database instance.
typedef struct { size_t padding; } Database;

/**
 * @brief The Cursor struct
 */
typedef struct { size_t padding; } Cursor;

/**
 * @brief The Ingestion Session struct
 */
typedef struct { size_t padding; } Session;

//! Search stats
typedef struct {
  struct {
    u64 n_times;        //< How many times interpolation search was performed
    u64 n_steps;        //< How many interpolation search steps was performed
    u64 n_overshoots;   //< Number of overruns
    u64 n_undershoots;  //< Number of underruns
    u64 n_matches;      //< Number of matches by interpolation search only
    u64 n_reduced_to_one_page;
    u64 n_page_in_core_checks;  //< Number of page in core checks
    u64 n_page_in_core_errors;  //< Number of page in core check errors
    u64 n_pages_in_core_found;  //< Number of page in core found
    u64 n_pages_in_core_miss;   //< Number of page misses
  } istats;
  struct {
    u64 n_times;  //< How many times binary search was performed
    u64 n_steps;  //< How many binary search steps was performed
  } bstats;
  struct {
    u64 fwd_bytes;  //< Number of scanned bytes in forward direction
    u64 bwd_bytes;  //< Number of scanned bytes in backward direction
  } scan;
} SearchStats;


//! Storage stats
typedef struct {
  u64 n_entries;   //< Total number of entries
  u64 n_volumes;   //< Total number of volumes
  u64 free_space;  //< Free space total
  u64 used_space;  //< Space in use
} StorageStats;


//-------------------
// Utility functions
//-------------------
/**
 * initialize
 */
void initialize();

/** Convert error code to error message.
 * Function returns pointer to statically allocated string
 * there is no need to free it.
 */
const char* error_message(int error_code);

//------------------------------
// Storage management functions
//------------------------------

/**
 * @brief Creates storage for new database on the hard drive
 * @param base_file_name database file name (excl suffix)
 * @param metadata_path path to metadata file
 * @param volumes_path path to volumes
 * @param num_volumes number of volumes to create
 */
EXPORT int create_database(const char* base_file_name, const char* metadata_path,
                           const char* volumes_path, i32 num_volumes, bool allocate);

/**
 * @brief Creates storage for new test database on the hard drive (smaller size then normal DB)
 * @param base_file_name database file name (excl suffix)
 * @param metadata_path path to metadata file
 * @param volumes_path path to volumes
 * @param num_volumes number of volumes to create
 */
EXPORT int create_database_ex(const char* base_file_name, const char* metadata_path,
                              const char* volumes_path, i32 num_volumes,
                              u64 page_size, bool allocate);

/** Remove all volumes.
 * @param file_name
 * @param logger
 * @returns status
 */
EXPORT int remove_database(const char* file_name, const char* wal_path, bool force);


/** Open recenlty create storage.
  * @param path path to storage metadata file
  * @param parameters open parameters
  * @return pointer to new db instance, null if db doesn't exists.
  */
EXPORT Database* open_database(const char* path, FineTuneParams parameters);


//! Close database. Free resources.
EXPORT void close_database(Database* db);


//-----------
// Ingestion
//-----------
EXPORT Session* create_session(Database* db);

EXPORT void destroy_session(Session* stream);

//---------
// Parsing
//---------

/** Try to parse timestamp.
 * @param iso_str should point to the begining of the string
 * @param sample is an output parameter
 * @returns SUCCESS on success, EBAD_ARG otherwise
 */
EXPORT int parse_timestamp(const char* iso_str, Sample* sample);

/** Convert series name to id. Assign new id to series name on first encounter.
 * @param ist is an opened ingestion stream
 * @param begin should point to the begining of the string
 * @param end should point to the next after end character of the string
 * @param sample is an output parameter
 * @returns SUCCESS on success, error code otherwise
 */
EXPORT int series_to_param_id(Session* ist, const char* begin, const char* end,
                       Sample* sample);

/**
  * Convert series name to id or list of ids (if metric name is composed from several metric names e.g. foo|bar)
  * @param ist is an opened ingestion stream
  * @param begin should point to the begining of the string
  * @param end should point to the next after end character of the string
  * @param out_ids is a destination array
  * @param out_ids_cap is a size of the dest array
  * @return number of elemnts stored in the out_ids array (can be less then out_ids_cap) or -1*number_of_series
  *         if dest is too small.
  */
EXPORT int name_to_param_id_list(Session* ist, const char* begin, const char* end,
                                         ParamId* out_ids, u32 out_ids_cap);
/** Try to parse duration.
  * @param str should point to the begining of the string
  * @param value is an output parameter
  * @returns SUCCESS on success, EBAD_ARG otherwise
  */
EXPORT int parse_duration(const char* str, int* value);


//---------
// Writing
//---------

/** Write measurement to DB
 * @param ist is an opened ingestion stream
 * @param param_id storage parameter id
 * @param timestamp timestamp
 * @param value parameter value
 * @returns operation status
 */
EXPORT int write_double_raw_sample(Session* session, ParamId param_id,
                                   Timestamp timestamp,  double value);

/** Write measurement to DB
 * @param ist is an opened ingestion stream
 * @param sample should contain valid measurement value
 * @returns operation status
 */
EXPORT int write_sample(Session* ist, const Sample* sample);


//---------
// Queries
//---------

/** @brief Query database
 * @param session should point to opened session instance
 * @param query should contain valid query
 * @return cursor instance
 */
EXPORT Cursor* query(Session* session, const char* query);

/** @brief Suggest query
 * @param sesson should point to opened session instance
 * @param query should contain valid query
 * @return cursor instance
 */
EXPORT Cursor* suggest(Session* session, const char* query);

/** @brief Search query
 * @param sesson should point to opened session instance
 * @param query should contain valid query
 * @return cursor instance
 */
EXPORT Cursor* search(Session* session, const char* query);

/**
 * @brief Close cursor
 * @param pcursor pointer to cursor
 */
EXPORT void cursor_close(Cursor* pcursor);

/** Read the values under cursor.
 * @param cursor should point to active cursor instance
 * @param dest is an output buffer
 * @param dest_size is an output buffer size
 * @returns number of overwriten bytes
 */
EXPORT size_t cursor_read(Cursor* cursor, void* dest, size_t dest_size);

//! Check cursor state. Returns zero value if not done yet, non zero value otherwise.
EXPORT int cursor_is_done(Cursor* pcursor);

//! Check cursor error state. Returns zero value if everything is OK, non zero value otherwise.
EXPORT int cursor_is_error(Cursor* pcursor);

/**
 * Check cursor error state and error message.
 * Returns zero value if everything is OK, non zero value otherwise.
 */
EXPORT int cursor_is_error_ex(Cursor*  pcursor,
                              const char** error_message);

/** Convert timestamp to string if possible, return string length
 * @return 0 on bad string, -LEN if buffer is too small, LEN on success
 */
EXPORT int timestamp_to_string(Timestamp, char* buffer, size_t buffer_size);

/** Convert param-id to series name
 * @param session
 * @param id valid param id
 * @param buffer is a destination buffer
 * @param buffer_size is a destination buffer size
 * @return 0 if no such id, -LEN if buffer is too small, LEN on success
 */
EXPORT int param_id_to_series(Session* session, ParamId id, char* buffer,
                              size_t buffer_size);

//--------------------
// Stats and counters
//--------------------

/** DEPRICATED
 * Get search counters.
 * @param rcv_stats pointer to `SearchStats` structure that will be filled with data.
 * @param reset reset all counter if not zero
 */
EXPORT void global_search_stats(SearchStats* rcv_stats, int reset);

/** DEPRICATED
 * Get storage stats.
 * @param db database instance.
 * @param rcv_stats pointer to destination
 */
EXPORT void global_storage_stats(Database* db, StorageStats* rcv_stats);

EXPORT void debug_print(Database* db);

EXPORT int json_stats(Database* db, char* buffer, size_t size);

/** Get global resource value by name
*/
EXPORT int get_resource(const char* res_name, char* buf, size_t* bufsize);

EXPORT int debug_report_dump(const char* path2db, const char* outfile);

EXPORT int debug_recovery_report_dump(const char* path2db, const char* outfile);

#endif  // FASTSTDB_FASTSTDB_H_
