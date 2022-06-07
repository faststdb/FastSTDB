/**
 * \file datetime.h
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
#ifndef FASTSTDB_COMMON_DATETIME_H_
#define FASTSTDB_COMMON_DATETIME_H_

#include <chrono>

#include <boost/date_time/posix_time/posix_time.hpp>

#include "faststdb/common/basic.h"

namespace faststdb {

using Duration = Timestamp;

/**
 * TimeStamp is a main datatype to represent date-time values.
 * It stores number of nanoseconds since epoch so it can fit u64 and doesn't prone to year 2038
 * problem.
 */

//! Timestamp parsing error
struct BadDateTimeFormat : std::runtime_error {
  BadDateTimeFormat(const char* str) : std::runtime_error(str) { }
};

//! Static utility class for date-time utility functions
struct DateTimeUtil {
  static Timestamp from_std_chrono(std::chrono::system_clock::time_point timestamp);

  static Timestamp from_boost_ptime(boost::posix_time::ptime timestamp);

  static boost::posix_time::ptime to_boost_ptime(Timestamp timestamp);

  /**
   * Convert ISO formatter timestamp to Timestamp value.
   * 
   * @note This function implements ISO 8601 partially compatible parser. Most of the standard is not
   * supported yet - extended formatting (only basic format is supported), fractions on minutes or hours (
   * like "20150102T1230.999"), timezones (values is treated as UTC time).
   *
   */
  static Timestamp from_iso_string(const char* iso_str);

  /**
   * Convert timestamp to string.
   */
  static int to_iso_string(Timestamp ts, char* buffer, size_t buffer_size);
  static std::string to_iso_string(Timestamp ts);

  /**
   * Parse time-duration from string
   * @throw BadDateTimeFormat on error
   */
  static Duration parse_duration(const char* str, size_t size);
};

}  // namespace faststdb

#endif  // FASTSTDB_COMMON_DATETIME_H_
