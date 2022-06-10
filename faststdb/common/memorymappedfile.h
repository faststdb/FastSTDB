/**
 * \file memorymappedfile.h
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
#ifndef FASTSTDB_COMMON_MEMORYMAPPEDFILE_H_
#define FASTSTDB_COMMON_MEMORYMAPPEDFILE_H_

#include <apr_general.h>
#include <apr_mmap.h>

#include <atomic>
#include <ostream>
#include <random>
#include <stdexcept>
#include <tuple>
#include <vector>

#include "faststdb/common/status.h"

namespace faststdb {

/** Memory mapped file
 * maps all file on construction
 */
class MemoryMappedFile {
  apr_pool_t*  mem_pool_;  //< local memory pool
  apr_mmap_t*  mmap_;
  apr_file_t*  fp_;
  apr_finfo_t  finfo_;
  apr_status_t status_;
  std::string  path_;
  const bool   enable_huge_tlb_;

 public:
  MemoryMappedFile(const char* file_name, bool enable_huge_tlb);
  ~MemoryMappedFile();
  void move_file(const char* new_name);
  void   delete_file();
  void*  get_pointer() const;
  size_t get_size() const;
  //! Flush only part of the page
  common::Status flush(size_t from, size_t to);
  //! Flush full page
  common::Status flush();
  bool         is_bad() const;
  std::string  error_message() const;
  void         panic_if_bad();
  apr_status_t status_code() const;
  //! Remap file in a destructive way (all file content is lost)
  void remap_file_destructive();
  //! Protect page from writing
  common::Status protect_all();
  //! Make page available for writing
  common::Status unprotect_all();

 private:
  //! Map file into virtual address space
  apr_status_t map_file();
  //! Free OS resources associated with object
  void free_resources(int cnt);
};

/** APR error converter */
std::string apr_error_message(apr_status_t status);

size_t get_page_size();

const void* align_to_page(const void* ptr, size_t get_page_size);

void* align_to_page(void* ptr, size_t get_page_size);

void prefetch_mem(const void* ptr, size_t mem_size);

}  // namespace faststdb

#endif  // FASTSTDB_COMMON_MEMORYMAPPEDFILE_H_

