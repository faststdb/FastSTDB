/*!
 * \file datetime_test.cc
 */
#include "faststdb/common/datetime.h"

#include "gtest/gtest.h"

namespace faststdb {

TEST(DateTime, Test_string_iso_to_timestamp_conversion) {
  const char* timestamp_str = "20060102T150405.999999999";  // ISO timestamp
  Timestamp actual = DateTimeUtil::from_iso_string(timestamp_str);
  Timestamp expected = 1136214245999999999ul;
  EXPECT_EQ(expected, actual);

  // To string
  const int buffer_size = 100;
  char buffer[buffer_size];
  int len = DateTimeUtil::to_iso_string(actual, buffer, buffer_size);

  EXPECT_EQ(26, len);
  EXPECT_STREQ(buffer, timestamp_str);
}

TEST(DateTime, Test_string_to_duration_seconds) {
  const char* test_case = "10s";
  Duration actual = DateTimeUtil::parse_duration(test_case, 3u);
  Duration expected = 10000000000ul;
  EXPECT_EQ(expected, actual);
}

TEST(DateTime, Test_string_to_duration_nanos) {
  const char* test_case = "111n";
  Duration actual = DateTimeUtil::parse_duration(test_case, 4u);
  Duration expected = 111ul;
  EXPECT_EQ(expected, actual);
}

TEST(DateTime, Test_string_to_duration_nanos2) {
  const char* test_case = "111";
  Duration actual = DateTimeUtil::parse_duration(test_case, 3u);
  Duration expected = 111ul;
  EXPECT_EQ(expected, actual);
}

TEST(DateTime, Test_string_to_duration_us) {
  const char* test_case = "111us";
  Duration actual = DateTimeUtil::parse_duration(test_case, 5u);
  Duration expected = 111000ul;
  EXPECT_EQ(expected, actual);
}

TEST(DateTime, Test_string_to_duration_ms) {
  const char* test_case = "111ms";
  Duration actual = DateTimeUtil::parse_duration(test_case, 5u);
  Duration expected = 111000000ul;
  EXPECT_EQ(expected, actual);
}

TEST(DateTime, Test_string_to_duration_minutes) {
  const char* test_case = "111m";
  Duration actual = DateTimeUtil::parse_duration(test_case, 4u);
  Duration expected = 111 * 60 * 1000000000ull;
  EXPECT_EQ(expected, actual);
}

}  // namespace faststdb
