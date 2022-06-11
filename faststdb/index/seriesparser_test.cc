/*!
 * \file seriesparser_test.cc
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
#include "faststdb/index/seriesparser.h"

#include "gtest/gtest.h"

namespace faststdb {

TEST(SeriesMatcher, Test_seriesmatcher_0) {
  SeriesMatcher matcher(1ul);
  const char* foo = "foo ba=r";
  const char* bar = "bar foo=bar";
  const char* buz = "buz b=uz";
  auto exp_foo = matcher.add(foo, foo + 8);
  auto exp_bar = matcher.add(bar, bar + 11);

  auto foo_id = matcher.match(foo, foo + 8);
  EXPECT_EQ(foo_id, 1ul);
  EXPECT_EQ(foo_id, exp_foo);

  auto bar_id = matcher.match(bar, bar + 11);
  EXPECT_EQ(bar_id, 2ul);
  EXPECT_EQ(bar_id, exp_bar);

  auto buz_id = matcher.match(buz, buz + 8);
  EXPECT_EQ(buz_id, 0ul);
}

TEST(SeriesParser, Test_seriesparser_0) {
  const char* series1 = " cpu  region=europe   host=127.0.0.1 ";
  auto len = strlen(series1);
  char out[40];
  const char* pbegin = nullptr;
  const char* pend = nullptr;
  auto status = SeriesParser::to_canonical_form(series1, series1 + len, out, out + len, &pbegin, &pend);

  EXPECT_EQ(common::Status::Ok(), status);

  std::string expected = "cpu host=127.0.0.1 region=europe";
  std::string actual = std::string(static_cast<const char*>(out), pend);
  EXPECT_STREQ(expected.c_str(), actual.c_str());

  std::string keystr = std::string(pbegin, pend);
  EXPECT_STREQ("host=127.0.0.1 region=europe", keystr.c_str());
}

TEST(SeriesParser, Test_seriesparser_1) {
  const char* series1 = "cpu";
  auto len = strlen(series1);
  char out[27];
  const char* pend = nullptr;
  auto status = SeriesParser::to_canonical_form(series1, series1 + len, out, out + len, &pend, &pend);
  EXPECT_EQ(common::Status::BadData(), status);
}

TEST(SeriesParser, Test_seriesparser_2) {
  const char* series1 = "cpu region host=127.0.0.1 ";
  auto len = strlen(series1);
  char out[27];
  const char* pend = nullptr;
  auto status = SeriesParser::to_canonical_form(series1, series1 + len, out, out + len, &pend, &pend);
  EXPECT_EQ(common::Status::BadData(), status);
}

TEST(SeriesParser, Test_seriesparser_3) {
  const char* series1 = "cpu region=europe host";
  auto len = strlen(series1);
  char out[27];
  const char* pend = nullptr;
  auto status = SeriesParser::to_canonical_form(series1, series1 + len, out, out + len, &pend, &pend);
  EXPECT_EQ(common::Status::BadData(), status);
}

TEST(SeriesParser, Test_seriesparser_4) {
  auto len = LIMITS_MAX_SNAME + 1;
  char series1[len];
  char out[len];
  const char* pend = nullptr;
  auto status = SeriesParser::to_canonical_form(series1, series1 + len, out, out + len, &pend, &pend);
  EXPECT_EQ(common::Status::BadData(), status);
}

TEST(SeriesParser, Test_seriesparser_5) {
  auto len = LIMITS_MAX_SNAME - 1;
  char series1[len];
  char out[10];
  const char* pend = nullptr;
  auto status = SeriesParser::to_canonical_form(series1, series1 + len, out, out + 10, &pend, &pend);
  EXPECT_EQ(common::Status::BadArg(), status);
}

TEST(SeriesParser, Test_seriesparser_6) {
  const char* tags[] = {
    "tag2",
    "tag4",
    "tag7",  // doesn't exists in series name
  };
  const char* series = "metric tag1=1 tag2=2 tag3=3 tag4=4 tag5=5";
  auto name = std::make_pair(series, strlen(series));
  char out[LIMITS_MAX_SNAME];
  common::Status status;
  SeriesParser::StringT result;
  StringTools::SetT filter = StringTools::create_set(2);
  filter.insert(std::make_pair(tags[0], 4));
  filter.insert(std::make_pair(tags[1], 4));
  filter.insert(std::make_pair(tags[2], 4));
  std::tie(status, result) = SeriesParser::filter_tags(name, filter, out);
  EXPECT_EQ(common::Status::Ok(), status);
  EXPECT_STREQ("metric tag2=2 tag4=4", std::string(result.first, result.first + result.second).c_str());
}

TEST(SeriesParser, Test_seriesparser_7) {
  const char* series1 = "cpu\\ user region=europe host=127.0.0.1";
  auto len = strlen(series1);
  char out[40];
  const char* pbegin = nullptr;
  const char* pend = nullptr;
  auto status = SeriesParser::to_canonical_form(series1, series1 + len, out, out + len, &pbegin, &pend);

  EXPECT_EQ(common::Status::Ok(), status);

  std::string expected = "cpu\\ user host=127.0.0.1 region=europe";
  std::string actual = std::string(static_cast<const char*>(out), pend);
  EXPECT_STREQ(expected.c_str(), actual.c_str());

  std::string keystr = std::string(pbegin, pend);
  EXPECT_STREQ("host=127.0.0.1 region=europe", keystr.c_str());
}

TEST(SeriesParser, Test_seriesparser_8) {

  const char* series1 = "cpu region=us\\ east host=127.0.0.1\\ aka\\ localhost\\ ";
  auto len = strlen(series1);
  char out[140];
  const char* pbegin = nullptr;
  const char* pend = nullptr;
  auto status = SeriesParser::to_canonical_form(series1, series1 + len, out, out + len, &pbegin, &pend);

  EXPECT_EQ(common::Status::Ok(), status);

  std::string expected = "cpu host=127.0.0.1\\ aka\\ localhost\\  region=us\\ east";
  std::string actual = std::string(static_cast<const char*>(out), pend);
  EXPECT_STREQ(expected.c_str(), actual.c_str());

  std::string keystr = std::string(pbegin, pend);
  EXPECT_STREQ("host=127.0.0.1\\ aka\\ localhost\\  region=us\\ east", keystr.c_str());
}

TEST(SeriesParser, Test_seriesparser_9) {
  const char* series1 = "\\ cpu\\user\\  \\ host\\ name=foo\\bar\\";
  auto len = strlen(series1);
  char out[0x140];
  const char* pbegin = nullptr;
  const char* pend = nullptr;
  auto status = SeriesParser::to_canonical_form(series1, series1 + len, out, out + len, &pbegin, &pend);

  EXPECT_EQ(common::Status::Ok(), status);

  std::string expected = "\\ cpu\\user\\  \\ host\\ name=foo\\bar\\";
  std::string actual = std::string(static_cast<const char*>(out), pend);
  EXPECT_STREQ(expected.c_str(), actual.c_str());

  std::string keystr = std::string(pbegin, pend);
  EXPECT_STREQ("\\ host\\ name=foo\\bar\\", keystr.c_str());
}

}  // namespace faststdb

