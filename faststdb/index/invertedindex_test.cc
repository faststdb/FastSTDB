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

#include "faststdb/index/seriesparser.h"

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

TEST(TestInvertedIndex, Test_1) {
  InvertedIndex inverted_index(1);
  for (auto i = 0; i < 100; ++i) {
    inverted_index.add(i + 1, i + 1);
  }
  inverted_index.add(20, 30000);

  auto z = inverted_index.extract(20);
  EXPECT_STREQ("20 30000 ", z.debug_string().c_str());
}

TEST(TestIndex, Test_1) {
  Index index;
  {
    const char* series = "metric tag1=1 tag2=2 tag3=3 tag4=4 tag5=5";
    auto ret = index.append(series, series + strlen(series));
    EXPECT_EQ(common::Status::Ok(), std::get<0>(ret));
  }
  {
    const char* series = "metric1 tag1=1";
    auto ret = index.append(series, series + strlen(series));
    EXPECT_EQ(common::Status::Ok(), std::get<0>(ret));
  }
  {
    const char* series = "metric tag1=2 tag2=2";
    auto ret = index.append(series, series + strlen(series));
    EXPECT_EQ(common::Status::Ok(), std::get<0>(ret));
  }

  // IncludeIfAllTagsMatch unit-test-1
  {
    std::vector<TagValuePair> tag_value_pairs;
    MetricName metric_name("metric");
    tag_value_pairs.emplace_back(TagValuePair("tag1=2"));
    IncludeIfAllTagsMatch include_all_tags_match(metric_name, tag_value_pairs.begin(), tag_value_pairs.end());
    auto index_query_results = include_all_tags_match.query(index);
    auto begin = index_query_results.begin();
    auto end = index_query_results.end();
    EXPECT_EQ(1, index_query_results.cardinality());
    for (; begin != end; ++begin) {
      std::stringstream ss;
      ss << *begin;
      EXPECT_STREQ("metric tag1=2 tag2=2", ss.str().c_str());
    }
  }

  // IncludeIfAllTagsMatch unit-test-2
  {
    std::vector<TagValuePair> tag_value_pairs;
    MetricName metric_name("metric");
    IncludeIfAllTagsMatch include_all_tags_match(metric_name, tag_value_pairs.begin(), tag_value_pairs.end());
    auto index_query_results = include_all_tags_match.query(index);
    EXPECT_EQ(0, index_query_results.cardinality());
  }

  // IncludeIfAllTagsMatch unit-test-3
  {
    std::vector<TagValuePair> tag_value_pairs;
    MetricName metric_name("metric");
    tag_value_pairs.emplace_back("tag2=2");
    IncludeIfAllTagsMatch include_all_tags_match(metric_name, tag_value_pairs.begin(), tag_value_pairs.end());
    auto index_query_results = include_all_tags_match.query(index);
    EXPECT_EQ(2, index_query_results.cardinality());
    auto begin = index_query_results.begin();
    {
      std::stringstream ss;
      ss << *begin;
      EXPECT_STREQ("metric tag1=1 tag2=2 tag3=3 tag4=4 tag5=5", ss.str().c_str());
    }
    ++begin;
    {
      std::stringstream ss;
      ss << *begin;
      EXPECT_STREQ("metric tag1=2 tag2=2", ss.str().c_str());
    }
  }

  // IncludeMany2Many unit-test-1 
  {
    std::map<std::string, std::vector<std::string>> tag_map;
    tag_map["tag1"].emplace_back("1");
    tag_map["tag1"].emplace_back("2");
    IncludeMany2Many include_many2many("metric", tag_map);
    auto index_query_results = include_many2many.query(index);
    // LOG(INFO) << "size=" << index_query_results.cardinality();
    EXPECT_EQ(2, index_query_results.cardinality());
    auto begin = index_query_results.begin();
    {
      std::stringstream ss;
      ss << *begin;
      EXPECT_STREQ("metric tag1=1 tag2=2 tag3=3 tag4=4 tag5=5", ss.str().c_str());
    }
    ++begin;
    {
      std::stringstream ss;
      ss << *begin;
      EXPECT_STREQ("metric tag1=2 tag2=2", ss.str().c_str());
    }
  }

  // IncludeIfHasTag unit-test-1
  {
    std::vector<std::string> tags = { "tag1" };
    IncludeIfHasTag include_if_has_tag("metric", tags);
    auto index_query_results = include_if_has_tag.query(index);
    EXPECT_EQ(2, index_query_results.cardinality());
    auto begin = index_query_results.begin();
    {
      std::stringstream ss;
      ss << *begin;
      EXPECT_STREQ("metric tag1=1 tag2=2 tag3=3 tag4=4 tag5=5", ss.str().c_str());
    }
    ++begin;
    {
      std::stringstream ss;
      ss << *begin;
      EXPECT_STREQ("metric tag1=2 tag2=2", ss.str().c_str());
    }
  }

  // IncludeIfHasTag unit-test-2
  {
    std::vector<std::string> tags = { "tag3" };
    IncludeIfHasTag include_if_has_tag("metric", tags);
    auto index_query_results = include_if_has_tag.query(index);
    EXPECT_EQ(1, index_query_results.cardinality());
    auto begin = index_query_results.begin();
    {
      std::stringstream ss;
      ss << *begin;
      EXPECT_STREQ("metric tag1=1 tag2=2 tag3=3 tag4=4 tag5=5", ss.str().c_str());
    }
  }

  // ExcludeTags unit-test-2 
  {
    std::vector<TagValuePair> tag_value_pairs;
    tag_value_pairs.emplace_back("tag1=1");
    ExcludeTags exclude_tags("metric", tag_value_pairs.begin(), tag_value_pairs.end());
    auto index_query_results = exclude_tags.query(index);
    EXPECT_EQ(1, index_query_results.cardinality());
    auto begin = index_query_results.begin();
    {
      std::stringstream ss;
      ss << *begin;
      EXPECT_STREQ("metric tag1=2 tag2=2", ss.str().c_str());
    }
  }

  // JoinByTags unit-test-1 
  {
    std::vector<MetricName> metrics;
    std::vector<TagValuePair> tag_value_pairs;
    metrics.emplace_back("metric");
    tag_value_pairs.emplace_back("tag1=2");
    tag_value_pairs.emplace_back("tag2=2");
    JoinByTags join_by_tags(metrics.begin(), metrics.end(), tag_value_pairs.begin(), tag_value_pairs.end());
    auto index_query_results = join_by_tags.query(index);
    EXPECT_EQ(2, index_query_results.cardinality());
    auto begin = index_query_results.begin();
    {
      std::stringstream ss;
      ss << *begin;
      EXPECT_STREQ("metric tag1=1 tag2=2 tag3=3 tag4=4 tag5=5", ss.str().c_str());
    }
    ++begin;
    {
      std::stringstream ss;
      ss << *begin;
      EXPECT_STREQ("metric tag1=2 tag2=2", ss.str().c_str());
    }
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

TEST(TestIndex, Test_index_0) {
  SeriesMatcher matcher(10ul);
  std::vector<std::string> names = {
    "foo tagA=1 tagB=1",
    "foo tagA=1 tagB=2",
    "foo tagA=1 tagB=3",
    "foo tagA=1 tagB=4",
    "foo tagA=2 tagB=1",
    "foo tagA=2 tagB=2",
    "foo tagA=2 tagB=3",
    "foo tagA=2 tagB=4",
  };
  std::vector<u64> ids;
  for (auto name: names) {
    auto id = matcher.add(name.data(), name.data() + name.size());
    EXPECT_TRUE(id != 0);
    ids.push_back(id);
  }
  MetricName mname("foo");
  std::vector<TagValuePair> tags = {
    TagValuePair("tagA=1")
  };
  IncludeIfAllTagsMatch query(mname, tags.begin(), tags.end());
  auto res = matcher.search(query);
  EXPECT_EQ(4, res.size());
  int i = 0;
  u64 exp_id = 10ul;
  for (auto tup: res) {
    const char* name;
    int size;
    u64 id;
    std::tie(name, size, id) = tup;
    EXPECT_STREQ(std::string(name, name + size).c_str(), names[i].c_str());
    EXPECT_EQ(id, exp_id);
    i++;
    exp_id++;
  }
}

TEST(TestIndex, Test_index_1) {
  u64 base_id = 10ul;
  SeriesMatcher matcher(base_id);
  std::vector<std::string> names = {
    "foo tagA=1 tagB=1",
    "foo tagA=1 tagB=2",
    "foo tagA=1 tagB=3",
    "foo tagA=1 tagB=4",
    "foo tagA=2 tagB=1",
    "foo tagA=2 tagB=2",
    "foo tagA=2 tagB=3",
    "foo tagA=2 tagB=4",
  };
  std::vector<u64> ids;
  for (auto name: names) {
    auto id = matcher.add(name.data(), name.data() + name.size());
    EXPECT_TRUE(id != 0);
    ids.push_back(id);
  }
  MetricName mname("foo");
  std::vector<TagValuePair> tags = {
    TagValuePair("tagA=2"),
    TagValuePair("tagB=3")
  };

  IncludeIfAllTagsMatch query(mname, tags.begin(), tags.end());
  auto res = matcher.search(query);
  EXPECT_EQ(1, res.size());
  int i = 0;
  std::vector<u64> offsets = {
    6,
  };
  for (auto tup: res) {
    const char* name;
    int size;
    u64 id;
    std::tie(name, size, id) = tup;
    std::string exp_name = names[offsets[i]];
    EXPECT_STREQ(std::string(name, name + size).c_str(), exp_name.c_str());
    EXPECT_EQ(id, base_id + offsets[i]);
    i++;
  }
}

TEST(TestIndex, Test_index_2) {
  u64 base_id = 10ul;
  SeriesMatcher matcher(base_id);
  std::vector<std::string> names = {
    "foo tagA=1 tagB=1",
    "foo tagA=1 tagB=2",
    "foo tagA=1 tagB=3",
    "foo tagA=1 tagB=4",
    "foo tagA=2 tagB=1",
    "foo tagA=2 tagB=2",
    "foo tagA=2 tagB=3",
    "foo tagA=2 tagB=4",
  };
  std::vector<u64> ids;
  for (auto name: names) {
    auto id = matcher.add(name.data(), name.data() + name.size());
    EXPECT_TRUE(id != 0);
    ids.push_back(id);
  }
  MetricName mname("bar");
  std::vector<TagValuePair> tags = {
    TagValuePair("tagA=1"),
  };

  IncludeIfAllTagsMatch query(mname, tags.begin(), tags.end());
  auto res = matcher.search(query);
  EXPECT_EQ(0, res.size());
}

TEST(TestIndex, Test_index_3) {
  u64 base_id = 10ul;
  SeriesMatcher matcher(base_id);
  std::vector<std::string> names = {
    "foo tagA=1 tagB=1 tagC=2",
    "foo tagA=1 tagB=2 tagD=1",
    "foo tagA=1 tagB=3 tagC=8",
    "foo tagA=1 tagB=4 tagC=2",
    "foo tagA=2 tagB=1 tagC=3",
    "foo tagA=2 tagB=2 tagD=0",
    "foo tagA=2 tagB=3 tagC=9",
    "foo tagA=2 tagB=4 tagC=4",
  };
  std::vector<u64> ids;
  for (auto name: names) {
    auto id = matcher.add(name.data(), name.data() + name.size());
    EXPECT_TRUE(id != 0);
    ids.push_back(id);
  }
  std::vector<TagValuePair> tags = {
    TagValuePair("tagD=2"),
  };
  std::vector<std::string> qtags = {"tagD"};
  IncludeIfHasTag query("foo", qtags);
  auto res = matcher.search(query);
  EXPECT_EQ(2, res.size());
  int i = 0;
  std::vector<u64> offsets = {
    1,
    5,
  };
  for (auto tup: res) {
    const char* name;
    int size;
    u64 id;
    std::tie(name, size, id) = tup;
    std::string exp_name = names[offsets[i]];
    EXPECT_STREQ(std::string(name, name + size).c_str(), exp_name.c_str());
    EXPECT_EQ(id, base_id + offsets[i]);
    i++;
  }
}

TEST(TestIndex, Test_index_4) {
  u64 base_id = 10ul;
  SeriesMatcher matcher(base_id);
  std::vector<std::string> names = {
    "foo tagA=1 tagB=1",
    "foo tagA=1 tagB=2",
    "foo tagA=1 tagB=3",
    "foo tagA=1 tagB=4",
    "foo tagA=2 tagB=1",
    "foo tagA=2 tagB=2",
    "foo tagA=2 tagB=3",
    "foo tagA=2 tagB=4",
  };
  std::vector<u64> ids;
  for (auto name: names) {
    auto id = matcher.add(name.data(), name.data() + name.size());
    EXPECT_TRUE(id != 0);
    ids.push_back(id);
  }
  std::string mname("foo");
  std::map<std::string, std::vector<std::string>> tags = {
    { "tagA", { "2" } },
    { "tagB", { "2", "3" } },
  };

  IncludeMany2Many query(mname, tags);
  auto res = matcher.search(query);
  EXPECT_EQ(2, res.size());
  int i = 0;
  std::vector<u64> offsets = {
    5,
    6,
  };
  for (auto tup: res) {
    const char* name;
    int size;
    u64 id;
    std::tie(name, size, id) = tup;
    std::string exp_name = names[offsets[i]];
    EXPECT_STREQ(std::string(name, name + size).c_str(), exp_name.c_str());
    EXPECT_EQ(id, base_id + offsets[i]);
    i++;
  }
}

}  // namespace faststdb

