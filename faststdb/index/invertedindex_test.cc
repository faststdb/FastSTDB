/*!
 * \file invertedindex_test.cc
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
#include "faststdb/index/invertedindex.h"

#include "gtest/gtest.h"

namespace faststdb {

StringPool stringpool;

TEST(TestCompressedPList, Test_1) {
  u64 id = 1;
  CompressedPList pList;
  for (auto i = 0; i < 1000; ++i) {
    pList.add(id++);
  }
  EXPECT_EQ(1000, pList.cardinality());

  id = 1;
  auto iter = pList.begin();
  for (auto i = 0; i < 1000; ++i, ++iter) {
    EXPECT_EQ(id++, *iter);
  }
}

TEST(TestCompressedPList, Test_2) {
  u64 id = 1;
  CompressedPList pList;
  for (auto i = 0; i < 1000; ++i, id += 3) {
    pList.add(id);
  }
  EXPECT_EQ(1000, pList.cardinality());

  id = 1;
  auto iter = pList.begin();
  for (auto i = 0; i < 1000; ++i, ++iter, id += 3) {
    EXPECT_EQ(id, *iter);
  }
  LOG(INFO) << "getSizeInBytes=" << pList.getSizeInBytes();
}

TEST(TestCompressedPList, Test_3) {
  u64 id = 1;
  CompressedPList pList;
  for (auto i = 0; i < 1000; ++i) {
    pList.add(id);
    id += i % 2;
  }
  EXPECT_EQ(1000, pList.cardinality());

  id = 1;
  auto iter = pList.begin();
  for (auto i = 0; i < 1000; ++i, ++iter) {
    EXPECT_EQ(id, *iter);
    id += i % 2;
  }

  id = 1;
  auto uniqPList = pList.unique();
  EXPECT_EQ(500, uniqPList.cardinality());
  auto iter2 = uniqPList.begin();
  for (auto i = 0; i < 500; ++i, ++iter2, ++id) {
    EXPECT_EQ(id, *iter2);
  }
}

TEST(TestSeriesNameTopology, Test_1) {
  SeriesNameTopology series_name_topology;
  const char* series = "metric tag1=1 tag2=2 tag3=3 tag4=4 tag5=5";
  auto id = stringpool.add(series, series + strlen(series));
  series_name_topology.add_name(stringpool.str(id));
  auto list_metric_names = series_name_topology.list_metric_names();
  EXPECT_EQ(1, list_metric_names.size());
  EXPECT_STREQ("metric", std::string(list_metric_names[0].first, list_metric_names[0].second).c_str());
  EXPECT_EQ(6, list_metric_names[0].second);
}

}  // namespace faststdb

