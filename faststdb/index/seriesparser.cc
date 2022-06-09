/**
 * \file seriesparser.cc
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
#include "faststdb/index/seriesparser.h"

#include <string>
#include <map>
#include <algorithm>
#include <regex>

#include "faststdb/common/logging.h"

namespace faststdb {

static const StringT EMPTY = std::make_pair(nullptr, 0);

SeriesMatcher::SeriesMatcher(i64 starting_id)
    : table(StringTools::create_table(0x1000))
    , series_id(starting_id)
{
  if (starting_id == 0u) {
    LOG(FATAL) << "Bad series ID";
  }
}

i64 SeriesMatcher::add(const char* begin, const char* end) {
  std::lock_guard<std::mutex> guard(mutex);
  auto prev_id = series_id++;
  auto id = prev_id;
  if (*begin == '!') {
    // Series name starts with ! which mean that we're dealing with event
    id = -1*id;
  }
  common::Status status;
  StringT sname;
  std::tie(status, sname) = index.append(begin, end);
  if (!status.IsOk()) {
    series_id = id;
    return 0;
  }
  auto tup = std::make_tuple(std::get<0>(sname), std::get<1>(sname), id);
  table[sname] = id;
  inv_table[id] = sname;
  names.push_back(tup);
  return id;
}

void SeriesMatcher::_add(std::string series, i64 id) {
  if (series.empty()) {
    return;
  }
  _add(series.data(), series.data() + series.size(), id);
}

void SeriesMatcher::_add(const char*  begin, const char* end, i64 id) {
  std::lock_guard<std::mutex> guard(mutex);
  common::Status status;
  StringT sname;
  std::tie(status, sname) = index.append(begin, end);
  // StatusUtil::throw_on_error(status);
  table[sname] = id;
  inv_table[id] = sname;
}

i64 SeriesMatcher::match(const char* begin, const char* end) const {
  int len = static_cast<int>(end - begin);
  StringT str = std::make_pair(begin, len);

  std::lock_guard<std::mutex> guard(mutex);
  auto it = table.find(str);
  if (it == table.end()) {
    return 0ul;
  }
  return it->second;
}

StringT SeriesMatcher::id2str(i64 tokenid) const {
  std::lock_guard<std::mutex> guard(mutex);
  auto it = inv_table.find(tokenid);
  if (it == inv_table.end()) {
    return EMPTY;
  }
  return it->second;
}

void SeriesMatcher::pull_new_names(std::vector<PlainSeriesMatcher::SeriesNameT> *buffer) {
  std::lock_guard<std::mutex> guard(mutex);
  std::swap(names, *buffer);
}

std::vector<i64> SeriesMatcher::get_all_ids() const {
  std::vector<i64> result;
  {
    std::lock_guard<std::mutex> guard(mutex);
    for (auto const &tup: inv_table) {
      result.push_back(tup.first);
    }
  }
  std::sort(result.begin(), result.end());
  return result;
}

std::vector<SeriesMatcher::SeriesNameT> SeriesMatcher::search(IndexQueryNodeBase const& query) const {
  std::vector<SeriesMatcher::SeriesNameT> result;
  std::lock_guard<std::mutex> guard(mutex);
  auto resultset = query.query(index);
  for (auto it = resultset.begin(); it != resultset.end(); ++it) {
    auto str = *it;
    auto fit = table.find(str);
    if (fit == table.end()) {
      LOG(FATAL) << "Invalid index state";
    }
    result.push_back(std::make_tuple(str.first, str.second, fit->second));
  }
  return result;
}

std::vector<StringT> SeriesMatcher::suggest_metric(std::string prefix) const {
  std::vector<StringT> results;
  std::lock_guard<std::mutex> guard(mutex);
  results = index.get_topology().list_metric_names();
  auto resit = std::remove_if(results.begin(), results.end(), [prefix](StringT val) {
    if (val.second < prefix.size()) {
      return true;
    }
    auto eq = std::equal(prefix.begin(), prefix.end(), val.first);
    return !eq;
  });
  results.erase(resit, results.end());
  return results;
}

std::vector<StringT> SeriesMatcher::suggest_tags(std::string metric, std::string tag_prefix) const {
  std::vector<StringT> results;
  std::lock_guard<std::mutex> guard(mutex);
  results = index.get_topology().list_tags(tostrt(metric));
  auto resit = std::remove_if(results.begin(), results.end(), [tag_prefix](StringT val) {
    if (val.second < tag_prefix.size()) {
       return true;
    }
    auto eq = std::equal(tag_prefix.begin(), tag_prefix.end(), val.first);
    return !eq;
  });
  results.erase(resit, results.end());
  return results;
}

std::vector<StringT> SeriesMatcher::suggest_tag_values(std::string metric, std::string tag, std::string value_prefix) const {
  std::vector<StringT> results;
  std::lock_guard<std::mutex> guard(mutex);
  results = index.get_topology().list_tag_values(tostrt(metric), tostrt(tag));
  auto resit = std::remove_if(results.begin(), results.end(), [value_prefix](StringT val) {
    if (val.second < value_prefix.size()) {
      return true;
    }
    auto eq = std::equal(value_prefix.begin(), value_prefix.end(), val.first);
    return !eq;
  });
  results.erase(resit, results.end());
  return results;
}

size_t SeriesMatcher::memory_use() const {
  return index.memory_use();
}

size_t SeriesMatcher::index_memory_use() const {
  return index.index_memory_use();
}

size_t SeriesMatcher::pool_memory_use() const {
  return index.pool_memory_use();
}

PlainSeriesMatcher::PlainSeriesMatcher(i64 starting_id)
    : table(StringTools::create_table(0x1000))
    , series_id(starting_id)
{
  if (starting_id == 0u) {
    LOG(FATAL) << "Bad series ID";
  }
}

i64 PlainSeriesMatcher::add(const char* begin, const char* end) {
  auto id = series_id++;
  StringT pstr = pool.add(begin, end);
  auto tup = std::make_tuple(std::get<0>(pstr), std::get<1>(pstr), id);
  std::lock_guard<std::mutex> guard(mutex);
  table[pstr] = id;
  inv_table[id] = pstr;
  names.push_back(tup);
  return id;
}

void PlainSeriesMatcher::_add(std::string series, i64 id) {
  if (series.empty()) {
    return;
  }
  const char* begin = &series[0];
  const char* end = begin + series.size();
  StringT pstr = pool.add(begin, end);
  std::lock_guard<std::mutex> guard(mutex);
  table[pstr] = id;
  inv_table[id] = pstr;
}

void PlainSeriesMatcher::_add(const char*  begin, const char* end, i64 id) {
  StringT pstr = pool.add(begin, end);
  std::lock_guard<std::mutex> guard(mutex);
  table[pstr] = id;
  inv_table[id] = pstr;
}

i64 PlainSeriesMatcher::match(const char* begin, const char* end) const {
  int len = static_cast<int>(end - begin);
  StringT str = std::make_pair(begin, len);

  std::lock_guard<std::mutex> guard(mutex);
  auto it = table.find(str);
  if (it == table.end()) {
    return 0ul;
  }
  return it->second;
}

StringT PlainSeriesMatcher::id2str(i64 tokenid) const {
  std::lock_guard<std::mutex> guard(mutex);
  auto it = inv_table.find(tokenid);
  if (it == inv_table.end()) {
    return EMPTY;
  }
  return it->second;
}

void PlainSeriesMatcher::pull_new_names(std::vector<PlainSeriesMatcher::SeriesNameT> *buffer) {
  std::lock_guard<std::mutex> guard(mutex);
  std::swap(names, *buffer);
}

std::vector<i64> PlainSeriesMatcher::get_all_ids() const {
  std::vector<i64> result;
  {
    std::lock_guard<std::mutex> guard(mutex);
    for (auto const &tup: inv_table) {
      result.push_back(tup.first);
    }
  }
  std::sort(result.begin(), result.end());
  return result;
}

std::vector<PlainSeriesMatcher::SeriesNameT> PlainSeriesMatcher::regex_match(const char* rexp) const {
  StringPoolOffset offset = {};
  size_t size = 0;
  return regex_match(rexp, &offset, &size);
}

std::vector<PlainSeriesMatcher::SeriesNameT> PlainSeriesMatcher::regex_match(const char* rexp, StringPoolOffset* offset, size_t *prevsize) const {
  std::vector<SeriesNameT> series;
  std::vector<LegacyStringPool::StringT> res = pool.regex_match(rexp, offset, prevsize);

  std::lock_guard<std::mutex> guard(mutex);
  std::transform(res.begin(), res.end(), std::back_inserter(series), [this](StringT s) {
    auto it = table.find(s);
    if (it == table.end()) {
      // We should always find id by string, otherwise - invariant is
      // broken (due to memory corruption most likely).
      LOG(FATAL) << "Invalid string-pool.";
    }
    return std::make_tuple(s.first, s.second, it->second);
  });
  return series;
}

//! Move pointer to the of the whitespace, return this pointer or end on error
static const char* skip_space(const char* p, const char* end) {
  while(p < end && (*p == ' ' || *p == '\t')) {
    p++;
  }
  return p;
}

static StringTools::StringT get_tag_name(const char* p, const char* end) {
  StringTools::StringT EMPTY = {nullptr, 0};
  auto begin = p;
  while(p < end && *p != '=' && *p != ' ' && *p != '\t') {
    p++;
  }
  if (p == end || *p != '=') {
    return EMPTY;
  }
  return {begin, p - begin};
}

static const char ESC_CHAR = '\\';

static const char* copy_until(const char* begin, const char* end, const char pattern, char** out) {
  if (begin == end) {
    return begin;
  }
  const char* escape_char = *begin == ESC_CHAR ? begin : nullptr;
  char* it_out = *out;
  while (true) {
    *it_out = *begin;
    it_out++;
    begin++;
    if (begin == end) {
      break;
    }
    if (*begin == pattern) {
      if (std::prev(begin) != escape_char) {
        break;
      }
    } else if (*begin == ESC_CHAR) {
      escape_char = begin;
    }
  }
  *out = it_out;
  return begin;
}

//! Move pointer to the beginning of the next tag, return this pointer or end on error
static const char* skip_tag(const char* begin, const char* end, bool *error) {
  const char* escape_char = nullptr;
  // skip until '='
  const char* p = begin;
  while (p < end) {
    if (*p == '=') {
      break;
    } else if (*p == ' ') {
      if (std::prev(p) != escape_char) {
        break;
      }
    } else if (*p == ESC_CHAR) {
      escape_char = p;
    }
    p++;
  }
  if (p == begin || p == end || *p != '=') {
    *error = true;
    return end;
  }
  // skip until ' '
  const char* c = p;
  while (c < end) {
    if (*c == ' ') {
      if (std::prev(c) != escape_char) {
        break;
      }
    } else if (*c == ESC_CHAR) {
      escape_char = c;
    }
    c++;
  }
  *error = c == p;
  return c;
}

common::Status SeriesParser::to_canonical_form(const char* begin, const char* end,
                                               char* out_begin, char* out_end,
                                               const char** keystr_begin,
                                               const char** keystr_end)
{
  // Verify args
  if (end < begin) {
    return common::Status::BadArg();
  }
  if (out_end < out_begin) {
    return common::Status::BadArg();
  }
  int series_name_len = end - begin;
  if (series_name_len > LIMITS_MAX_SNAME) {
    return common::Status::BadData();
  }
  if (series_name_len > (out_end - out_begin)) {
    return common::Status::BadArg();
  }

  char* it_out = out_begin;
  const char* it = begin;
  // Get metric name
  it = skip_space(it, end);
  it = copy_until(it, end, ' ', &it_out);
  it = skip_space(it, end);

  if (it == end) {
    // At least one tag should be specified
    return common::Status::BadData();
  }

  *keystr_begin = it_out;

  // Get pointers to the keys
  const char* tags[LIMITS_MAX_TAGS];
  auto ix_tag = 0u;
  bool error = false;
  while (it < end && ix_tag < LIMITS_MAX_TAGS) {
    tags[ix_tag] = it;
    it = skip_tag(it, end, &error);
    it = skip_space(it, end);
    if (!error) {
      ix_tag++;
    } else {
      break;
    }
  }
  if (error) {
    // Bad string
    return common::Status::BadData();
  }
  if (ix_tag == 0) {
    // User should specify at least one tag
    return common::Status::BadData();
  }

  auto sort_pred = [end](const char* lhs, const char* rhs) {
    // lhs should be always less thenn rhs
    auto lenl = 0u;
    auto lenr = 0u;
    if (lhs < rhs) {
      lenl = rhs - lhs;
      lenr = end - rhs;
    } else {
      lenl = end - lhs;
      lenr = lhs - rhs;
    }
    auto it = 0u;
    while (true) {
      if (it >= lenl || it >= lenr) {
        return it < lenl;
      }
      if (lhs[it] == '=' || rhs[it] == '=') {
        return lhs[it] == '=' && rhs[it] != '=';
      }
      if (lhs[it] < rhs[it]) {
        return true;
      } else if (lhs[it] > rhs[it]) {
        return false;
      }
      it++;
    }
    return true;
  };
  std::sort(tags, tags + ix_tag, std::ref(sort_pred));  // std::sort can't move from predicate if predicate is a rvalue
  // nor it can pass predicate by reference
  // Copy tags to output string
  for (auto i = 0u; i < ix_tag; i++) {
    // insert space
    *it_out++ = ' ';
    // insert tag
    const char* tag = tags[i];
    copy_until(tag, end, ' ', &it_out);
  }
  *keystr_begin = skip_space(*keystr_begin, out_end);
  *keystr_end = it_out;
  return common::Status::Ok();
}

std::tuple<common::Status, SeriesParser::StringT> SeriesParser::filter_tags(SeriesParser::StringT const& input, const StringTools::SetT &tags, char* out, bool inv) {
  StringT NO_RESULT = {};
  char* out_begin = out;
  char* it_out = out;
  const char* it = input.first;
  const char* end = it + input.second;

  // Get metric name
  it = skip_space(it, end);
  it = copy_until(it, end, ' ', &it_out);
  it = skip_space(it, end);

  if (it == end) {
    // At least one tag should be specified
    return std::make_tuple(common::Status::BadData(), NO_RESULT);
  }

  // Get pointers to the keys
  const char* last_tag;
  auto ix_tag = 0u;
  bool error = false;
  while(it < end && ix_tag < LIMITS_MAX_TAGS) {
    last_tag = it;
    it = skip_tag(it, end, &error);
    if (!error) {
      // Check tag
      StringT tag = get_tag_name(last_tag, it);
      auto ntags = tags.count(tag);
      if ((!inv && ntags) || (inv && !ntags)) {
        *it_out = ' ';
        it_out++;
        auto sz = it - last_tag;
        memcpy((void*)it_out, (const void*)last_tag, sz);
        it_out += sz;
        ix_tag++;
      }
    } else {
      break;
    }
    it = skip_space(it, end);
  }

  if (error) {
    // Bad string
    return std::make_tuple(common::Status::BadData(), NO_RESULT);
  }

  if (ix_tag == 0) {
    // User should specify at least one tag
    return std::make_tuple(common::Status::BadData(), NO_RESULT);
  }

  return std::make_tuple(common::Status::Ok(), std::make_pair(out_begin, it_out - out_begin));
}

GroupByTag::GroupByTag(const SeriesMatcher &matcher, std::string metric, std::vector<std::string> const& tags, GroupByOpType op)
    : matcher_(matcher)
    , offset_{}
    , prev_size_(0)
    , metrics_({metric})
    , funcs_()
    , tags_(tags)
    , local_matcher_(1ul)
    , snames_(StringTools::create_set(64))
    , type_(op)
{
  refresh_();
}

GroupByTag::GroupByTag(const SeriesMatcher &matcher,
                       const std::vector<std::string>& metrics,
                       const std::vector<std::string> &func_names,
                       std::vector<std::string> const& tags,
                       GroupByOpType op)
    : matcher_(matcher)
    , offset_{}
    , prev_size_(0)
    , metrics_(metrics)
    , funcs_(func_names)
    , tags_(tags)
    , local_matcher_(1ul)
    , snames_(StringTools::create_set(64))
    , type_(op)
{
  refresh_();
}

PlainSeriesMatcher &GroupByTag::get_series_matcher() {
  return local_matcher_;
}

std::unordered_map<ParamId, ParamId> GroupByTag::get_mapping() const {
  return ids_;
}

void GroupByTag::refresh_() {
  int mindex = 0;
  for (auto metric: metrics_) {
    IncludeIfHasTag tag_query(metric, tags_);
    auto results = matcher_.search(tag_query);
    auto filter = StringTools::create_set(tags_.size());
    for (const auto& tag: tags_) {
      filter.insert(std::make_pair(tag.data(), tag.size()));
    }
    char buffer[LIMITS_MAX_SNAME];
    for (auto item: results) {
      common::Status status;
      SeriesParser::StringT result, stritem;
      stritem = std::make_pair(std::get<0>(item), std::get<1>(item));
      std::tie(status, result) = SeriesParser::filter_tags(stritem, filter, buffer, type_ == GroupByOpType::GROUP);
      if (status.IsOk()) {
        if (funcs_.size() != 0) {
          // Update metric name using aggregate function, e.g. cpu key=val -> cpu:max key=val
          const auto& fname = funcs_.at(mindex);
          std::string name(result.first, result.first + result.second);
          auto pos = name.find_first_of(' ');
          if (pos != std::string::npos) {
            std::string str = name.substr(0, pos) + ":" + fname + name.substr(pos);
            std::copy(str.begin(), str.end(), buffer);
            result = std::make_pair(buffer, static_cast<u32>(str.size()));
          }
        }
        if (snames_.count(result) == 0) {
          // put result to local stringpool and ids list
          auto localid = local_matcher_.add(result.first, result.first + result.second);
          auto str = local_matcher_.id2str(localid);
          snames_.insert(str);
          ids_[static_cast<ParamId>(std::get<2>(item))] = static_cast<ParamId>(localid);
        } else {
          // local name already created
          auto localid = local_matcher_.match(result.first, result.first + result.second);
          if (localid == 0ul) {
            LOG(FATAL) << "inconsistent matcher state";
          }
          ids_[static_cast<ParamId>(std::get<2>(item))] = static_cast<ParamId>(localid);
        }
      }
    }
    mindex++;
  }
}

}  // namespace faststdb
