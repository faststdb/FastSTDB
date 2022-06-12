/**
 * \file stringpool.h
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
#ifndef FASTSTDB_INDEX_STRINGPOOL_H_
#define FASTSTDB_INDEX_STRINGPOOL_H_

#include <atomic>
#include <deque>
#include <iostream>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <tsl/robin_map.h>
#include <tsl/robin_set.h>

#include "faststdb/common/basic.h"

namespace faststdb {

//! Offset inside string-pool
struct StringPoolOffset {
  //! Offset of the buffer
  size_t buffer_offset;
  //! Offset inside buffer
  size_t offset;
};

struct LegacyStringPool {
  typedef std::pair<const char*, int> StringT;
  const int MAX_BIN_SIZE = LIMITS_MAX_SNAME * 0x1000;

  std::deque<std::vector<char>> pool;
  mutable std::mutex            pool_mutex;
  std::atomic<size_t>           counter;

  LegacyStringPool();
  LegacyStringPool(LegacyStringPool const&) = delete;
  LegacyStringPool& operator=(LegacyStringPool const&) = delete;

  StringT add(const char* begin, const char* end);

  //! Get number of stored strings atomically
  size_t size() const;

  /** Find all series that match regex.
   * @param regex is a regullar expression
   * @param outoffset can be used to retreive offset of the processed data or start search from
   *        particullar point in the string-pool
   */
  std::vector<StringT> regex_match(const char* regex, StringPoolOffset* outoffset = nullptr,
                                   size_t* psize = nullptr) const;
};

typedef std::pair<const char*, u32> StringT;
inline std::ostream& operator<<(std::ostream& os, const StringT& data) {
  os << std::string(data.first, data.second);
  return os;
}

class StringPool {
 public:
  const u64 MAX_BIN_SIZE = LIMITS_MAX_SNAME * 0x1000;  // 8Mb

  std::deque<std::vector<char>> pool;
  mutable std::mutex            pool_mutex;
  std::atomic<size_t>           counter;

  StringPool();
  StringPool(StringPool const&) = delete;
  StringPool& operator=(StringPool const&) = delete;

  /**
   * @brief add value to string pool
   * @param begin is a pointer to the begining of the string
   * @param end is a pointer to the next character after the end of the string
   * @return Z-order encoded address of the string (0 in case of error)
   */
  u64 add(const char* begin, const char* end);

  /**
   * @brief str returns string representation
   * @param bits is a Z-order encoded position in the string buffer
   * @return 0-copy string representation (or empty string)
   */
  StringT str(u64 bits) const;

  //! Get number of stored strings atomically
  size_t size() const;

  size_t mem_used() const;
};

#ifdef USE_STD_HASHMAP
#define MapClass std::unordered_map
#define SetClass std::unordered_set
#else
#define MapClass tsl::robin_map
#define SetClass tsl::robin_set
#endif

struct StringTools {
  //! Pooled string
  typedef std::pair<const char*, int> StringT;

  static size_t hash(StringT str);
  static bool equal(StringT lhs, StringT rhs);

  struct Hash {
    std::size_t operator()(StringT const& str) const noexcept {
      return StringTools::hash(str);
    }
  };

  struct EqualTo {
    bool operator()(const StringT& lhs, const StringT& rhs) const {
      return StringTools::equal(lhs, rhs);
    }
  };

  typedef MapClass<StringT, i64, StringTools::Hash, StringTools::EqualTo>      TableT;
  typedef std::shared_ptr<TableT> TableTPtr;
  typedef SetClass<StringT, StringTools::Hash, StringTools::EqualTo>           SetT;
  typedef std::shared_ptr<SetT> SetTPtr;
  typedef MapClass<StringT, SetTPtr, StringTools::Hash, StringTools::EqualTo>     L2TableT;
  typedef std::shared_ptr<L2TableT> L2TableTPtr;
  typedef MapClass<StringT, L2TableTPtr, StringTools::Hash, StringTools::EqualTo> L3TableT;
  typedef std::shared_ptr<L3TableT> L3TableTPtr;
  typedef MapClass<i64, StringT>      InvT;

  static TableT create_table(size_t size);
  static TableTPtr create_table_ptr(size_t size);
  static SetT create_set(size_t size);
  static SetTPtr create_set_ptr(size_t size); 
  static L2TableT create_l2_table(size_t size_hint);
  static L2TableTPtr create_l2_table_ptr(size_t size_hint);
  static L3TableT create_l3_table(size_t size_hint);
  static L3TableTPtr create_l3_table_ptr(size_t size_hint);
};

}  // namespace faststdb

#endif  // FASTSTDB_INDEX_STRINGPOOL_H_
