/*!
 * \file merge.h
 */
#ifndef FASTSTDB_STORAGE_OPERATORS_MERGE_H_
#define FASTSTDB_STORAGE_OPERATORS_MERGE_H_

#include "faststdb/storage/operators/operator.h"

#include <boost/heap/skew_heap.hpp>
#include <boost/range.hpp>
#include <boost/range/iterator_range.hpp>

#include "faststdb/common/basic.h"
#include "faststdb/common/logging.h"

namespace faststdb {
namespace storage {

template <int dir, class ValueT>  // 0 - forward, 1 - backward
struct TimeOrderImpl {
  typedef std::tuple<Timestamp, ParamId> KeyType;
  typedef std::tuple<KeyType, ValueT, u32> HeapItem;
  std::greater<KeyType> greater_;
  std::less<KeyType> less_;

  bool operator () (HeapItem const& lhs, HeapItem const& rhs) const {
    if (dir == 0) {
      return greater_(std::get<0>(lhs), std::get<0>(rhs));
    }
    return less_(std::get<0>(lhs), std::get<0>(rhs));
  }
};

template <int dir>
using TimeOrder = TimeOrderImpl<dir, double>;

template <int dir>
using EventTimeOrder = TimeOrderImpl<dir, std::string>;


/**
 * This predicate is used by the join materializer.
 * Merge join should preserve order of the series supplied by the user.
 */
template <int dir>  // 0 - forward, 1 - backward
struct MergeJoinOrder {
  typedef std::tuple<Timestamp, ParamId> KeyType;
  typedef std::tuple<KeyType, double, u32> HeapItem;

  bool operator () (HeapItem const& lhs, HeapItem const& rhs) const {
    if (dir == 0) {
      return std::get<0>(std::get<0>(lhs)) > std::get<0>(std::get<0>(rhs));
    }
    return std::get<0>(std::get<0>(lhs)) < std::get<0>(std::get<0>(rhs));
  }
};


template <int dir, class ValueT>  // 0 - forward, 1 - backward
struct SeriesOrderImpl {
  typedef std::tuple<Timestamp, ParamId> KeyType;
  typedef std::tuple<KeyType, ValueT, u32> HeapItem;
  typedef std::tuple<ParamId, Timestamp> InvKeyType;
  std::greater<InvKeyType> greater_;
  std::less<InvKeyType> less_;

  bool operator () (HeapItem const& lhs, HeapItem const& rhs) const {
    auto lkey = std::get<0>(lhs);
    InvKeyType ilhs = std::make_tuple(std::get<1>(lkey), std::get<0>(lkey));
    auto rkey = std::get<0>(rhs);
    InvKeyType irhs = std::make_tuple(std::get<1>(rkey), std::get<0>(rkey));
    if (dir == 0) {
      return greater_(ilhs, irhs);
    }
    return less_(ilhs, irhs);
  }
};


template<int dir>
using SeriesOrder = SeriesOrderImpl<dir, double>;

template<int dir>
using EventSeriesOrder = SeriesOrderImpl<dir, std::string>;


template <template <int dir> class CmpPred, bool IsStable = false>
struct MergeMaterializer : ColumnMaterializer {
  std::vector<std::unique_ptr<RealValuedOperator>> iters_;
  std::vector<ParamId> ids_;
  bool forward_;

  enum {
    RANGE_SIZE=1024
  };

  struct Range {
    std::vector<Timestamp> ts;
    std::vector<double> xs;
    ParamId id;
    size_t size;
    size_t pos;

    Range(ParamId id)
        : id(id)
          , size(0)
          , pos(0)
    {
      ts.resize(RANGE_SIZE);
      xs.resize(RANGE_SIZE);
    }

    void advance() {
      pos++;
    }

    void retreat() {
      assert(pos);
      pos--;
    }

    bool empty() const {
      return !(pos < size);
    }

    std::tuple<Timestamp, ParamId> top_key() const {
      return std::make_tuple(ts.at(pos), id);
    }

    double top_value() const {
      return xs.at(pos);
    }
  };

  std::vector<Range> ranges_;

  MergeMaterializer(std::vector<ParamId>&& ids, std::vector<std::unique_ptr<RealValuedOperator>>&& it)
      : iters_(std::move(it))
        , ids_(std::move(ids))
        , forward_(true)
  {
    if (!iters_.empty()) {
      forward_ = iters_.front()->get_direction() == RealValuedOperator::Direction::FORWARD;
    }
    if (iters_.size() != ids_.size()) {
      LOG(FATAL) << "MergeIterator - broken invariant";
    }
  }

  virtual std::tuple<common::Status, size_t> read(u8* dest, size_t size) override {
    if (forward_) {
      return kway_merge<0>(dest, size);
    }
    return kway_merge<1>(dest, size);
  }

  template<int dir>
  std::tuple<common::Status, size_t> kway_merge(u8* dest, size_t size) {
    if (iters_.empty()) {
      return std::make_tuple(common::Status::NoData(""), 0);
    }
    size_t outpos = 0;
    if (ranges_.empty()) {
      // `ranges_` array should be initialized on first call
      for (size_t i = 0; i < iters_.size(); i++) {
        Range range(ids_[i]);
        common::Status status;
        size_t outsize;
        std::tie(status, outsize) = iters_[i]->read(range.ts.data(), range.xs.data(), RANGE_SIZE);
        if (status.IsOk() || (status.Code() == common::Status::kNoData && outsize != 0)) {
          range.size = outsize;
          range.pos  = 0;
          ranges_.push_back(std::move(range));
        }
        if (!status.IsOk() && (status.Code() != common::Status::kNoData)) {
          return std::make_tuple(status, 0);
        }
      }
    }

    typedef CmpPred<dir> Comp;
    typedef typename Comp::HeapItem HeapItem;
    typedef typename Comp::KeyType KeyType;
    typedef boost::heap::skew_heap<HeapItem, boost::heap::compare<Comp>, boost::heap::stable<IsStable>> Heap;
    Heap heap;

    int index = 0;
    for(auto& range: ranges_) {
      if (!range.empty()) {
        KeyType key = range.top_key();
        heap.push(std::make_tuple(key, range.top_value(), index));
      }
      index++;
    }

    enum {
      KEY = 0,
      VALUE = 1,
      INDEX = 2,
      TIME = 0,
      ID = 1,
    };

    while(!heap.empty()) {
      HeapItem item = heap.top();
      KeyType point = std::get<KEY>(item);
      u32 index = std::get<INDEX>(item);
      Sample sample;
      sample.paramid = std::get<ID>(point);
      sample.timestamp = std::get<TIME>(point);
      sample.payload.type = PAYLOAD_FLOAT;
      sample.payload.size = sizeof(Sample);
      sample.payload.float64 = std::get<VALUE>(item);
      if (size - outpos >= sizeof(Sample)) {
        memcpy(dest + outpos, &sample, sizeof(sample));
        outpos += sizeof(sample);
      } else {
        // Output buffer is fully consumed
        return std::make_tuple(common::Status::Ok(), outpos);
      }
      heap.pop();
      ranges_[index].advance();
      if (ranges_[index].empty()) {
        // Refill range if possible
        common::Status status;
        size_t outsize;
        std::tie(status, outsize) = iters_[index]->read(ranges_[index].ts.data(), ranges_[index].xs.data(), RANGE_SIZE);
        if (!status.IsOk() && (status.Code() != common::Status::kNoData)) {
          return std::make_tuple(status, 0);
        }
        ranges_[index].size = outsize;
        ranges_[index].pos  = 0;
      }
      if (!ranges_[index].empty()) {
        KeyType point = ranges_[index].top_key();
        heap.push(std::make_tuple(point, ranges_[index].top_value(), index));
      }
    }
    if (heap.empty()) {
      iters_.clear();
      ranges_.clear();
    }
    // All iterators are fully consumed
    return std::make_tuple(common::Status::NoData(""), outpos);
  }
};

template<template <int dir> class CmpPred, bool IsStable = false>
struct MergeEventMaterializer : ColumnMaterializer {
  std::vector<std::unique_ptr<BinaryDataOperator>> iters_;
  std::vector<ParamId> ids_;
  bool forward_;

  enum {
    RANGE_SIZE=1024
  };

  struct Range {
    std::vector<Timestamp> ts;
    std::vector<std::string> xs;
    ParamId id;
    size_t size;
    size_t pos;

    Range(ParamId id)
        : id(id)
          , size(0)
          , pos(0)
    {
      ts.resize(RANGE_SIZE);
      xs.resize(RANGE_SIZE);
    }

    void advance() {
      pos++;
    }

    void retreat() {
      assert(pos);
      pos--;
    }

    bool empty() const {
      return !(pos < size);
    }

    std::tuple<Timestamp, ParamId> top_key() const {
      return std::make_tuple(ts.at(pos), id);
    }

    std::string top_value() const {
      return xs.at(pos);
    }
  };

  std::vector<Range> ranges_;

  MergeEventMaterializer(std::vector<ParamId>&& ids, std::vector<std::unique_ptr<BinaryDataOperator>>&& it)
      : iters_(std::move(it))
        , ids_(std::move(ids))
        , forward_(true)
  {
    if (!iters_.empty()) {
      forward_ = iters_.front()->get_direction() == BinaryDataOperator::Direction::FORWARD;
    }
    if (iters_.size() != ids_.size()) {
      LOG(FATAL) << "MergeIterator - broken invariant";
    }
  }

  virtual std::tuple<common::Status, size_t> read(u8* dest, size_t size) override {
    if (forward_) {
      return kway_merge<0>(dest, size);
    }
    return kway_merge<1>(dest, size);
  }

  template<int dir>
  std::tuple<common::Status, size_t> kway_merge(u8* dest, size_t size) {
    if (iters_.empty()) {
      return std::make_tuple(common::Status::NoData(""), 0);
    }
    size_t outpos = 0;
    if (ranges_.empty()) {
      // `ranges_` array should be initialized on first call
      for (size_t i = 0; i < iters_.size(); i++) {
        Range range(ids_[i]);
        common::Status status;
        size_t outsize;
        std::tie(status, outsize) = iters_[i]->read(range.ts.data(), range.xs.data(), RANGE_SIZE);
        if (status.IsOk() || (status.Code() == common::Status::kNoData && outsize != 0)) {
          range.size = outsize;
          range.pos  = 0;
          ranges_.push_back(std::move(range));
        }
        if (!status.IsOk() && (status.Code() != common::Status::kNoData)) {
          return std::make_tuple(status, 0);
        }
      }
    }

    typedef CmpPred<dir> Comp;
    typedef typename Comp::HeapItem HeapItem;
    typedef typename Comp::KeyType KeyType;
    typedef boost::heap::skew_heap<HeapItem, boost::heap::compare<Comp>, boost::heap::stable<IsStable>> Heap;
    Heap heap;

    int index = 0;
    for(auto& range: ranges_) {
      if (!range.empty()) {
        KeyType key = range.top_key();
        heap.push(std::make_tuple(key, range.top_value(), index));
      }
      index++;
    }

    enum {
      KEY = 0,
      VALUE = 1,
      INDEX = 2,
      TIME = 0,
      ID = 1,
    };

    while(!heap.empty()) {
      HeapItem item = heap.top();
      KeyType point = std::get<KEY>(item);
      u32 index = std::get<INDEX>(item);

      // Produce sample

      std::string const& evt = std::get<VALUE>(item);
      // Check size
      u16 size_required = sizeof(Sample) + evt.size();

      Sample sample;
      sample.paramid = std::get<ID>(point);
      sample.timestamp = std::get<TIME>(point);
      sample.payload.type = PAYLOAD_EVENT;
      sample.payload.size = size_required;
      sample.payload.float64 = 0;
      if (size - outpos >= size_required) {
        auto psample = reinterpret_cast<Sample*>(dest + outpos);
        memcpy(psample, &sample, sizeof(sample));
        memcpy(psample->payload.data, evt.data(), evt.size());
        outpos += size_required;
      } else {
        // Output buffer is fully consumed
        return std::make_tuple(common::Status::Ok(), outpos);
      }
      heap.pop();
      ranges_[index].advance();
      if (ranges_[index].empty()) {
        // Refill range if possible
        common::Status status;
        size_t outsize;
        std::tie(status, outsize) = iters_[index]->read(ranges_[index].ts.data(), ranges_[index].xs.data(), RANGE_SIZE);
        if (!status.IsOk() && (status.Code() != common::Status::kNoData)) {
          return std::make_tuple(status, 0);
        }
        ranges_[index].size = outsize;
        ranges_[index].pos  = 0;
      }
      if (!ranges_[index].empty()) {
        KeyType point = ranges_[index].top_key();
        heap.push(std::make_tuple(point, ranges_[index].top_value(), index));
      }
    }
    if (heap.empty()) {
      iters_.clear();
      ranges_.clear();
    }
    // All iterators are fully consumed
    return std::make_tuple(common::Status::NoData(""), outpos);
  }
};

namespace MergeJoinUtil {
namespace {

std::tuple<Timestamp, ParamId> make_tord(Sample const* s) {
  return std::make_tuple(s->timestamp, s->paramid);
}

std::tuple<ParamId, Timestamp> make_sord(Sample const* s) {
  return std::make_tuple(s->paramid, s->timestamp);
}

}  // namespace

template<int dir, class TKey, TKey (*fnmake)(const Sample*)>  // TKey expected to be tuple
struct OrderBy {
  typedef TKey KeyType;
  struct HeapItem {
    KeyType key;
    Sample const* sample;
    size_t index;
  };
  std::greater<KeyType> greater_;
  std::less<KeyType> less_;

  bool operator () (HeapItem const& lhs, HeapItem const& rhs) const {
    if (dir == 0) {
      return greater_(lhs.key, rhs.key);
    }
    return less_(lhs.key, rhs.key);
  }

  static KeyType make_key(Sample const* sample) {
    return fnmake(sample);
  }
};

template<int dir>
using OrderByTimestamp = OrderBy<dir, std::tuple<Timestamp, ParamId>, &make_tord>;

template<int dir>
using OrderBySeries = OrderBy<dir, std::tuple<ParamId, Timestamp>, &make_sord>;

}  // namespace MergeJoinUtil 

/**
 * Merges several materialized tuple sequences into one
 */
template <template <int dir> class CmpPred>
struct MergeJoinMaterializer : ColumnMaterializer {

  enum {
    RANGE_SIZE=1024
  };

  struct Range {
    std::vector<u8> buffer;
    u32 size;
    u32 pos;
    u32 last_advance;

    Range()
        : size(0u)
          , pos(0u)
          , last_advance(0u)
    {
      buffer.resize(RANGE_SIZE * sizeof(Sample));
    }

    void advance(u32 sz) {
      pos += sz;
      last_advance = sz;
    }

    void retreat() {
      assert(pos > last_advance);
      pos -= last_advance;
    }

    bool empty() const {
      return !(pos < size);
    }

    std::tuple<Timestamp, ParamId> top_key() const {
      u8 const* top = buffer.data() + pos;
      Sample const* sample = reinterpret_cast<Sample const*>(top);
      return CmpPred<0>::make_key(sample);  // Direction doesn't matter here
    }

    Sample const* top() const {
      u8 const* top = buffer.data() + pos;
      return reinterpret_cast<Sample const*>(top);
    }
  };

  std::vector<std::unique_ptr<ColumnMaterializer>> iters_;
  bool forward_;
  std::vector<Range> ranges_;

  MergeJoinMaterializer(std::vector<std::unique_ptr<ColumnMaterializer>>&& it, bool forward)
      : iters_(std::move(it))
        , forward_(forward)
  {
  }

  virtual std::tuple<common::Status, size_t> read(u8* dest, size_t size) {
    if (forward_) {
      return kway_merge<0>(dest, size);
    }
    return kway_merge<1>(dest, size);
  }

  template<int dir>
  std::tuple<common::Status, size_t> kway_merge(u8* dest, size_t size) {
    if (iters_.empty()) {
      return std::make_tuple(common::Status::NoData(""), 0);
    }
    size_t outpos = 0;
    if (ranges_.empty()) {
      // `ranges_` array should be initialized on first call
      for (size_t i = 0; i < iters_.size(); i++) {
        Range range;
        common::Status status;
        size_t outsize;
        std::tie(status, outsize) = iters_[i]->read(range.buffer.data(), range.buffer.size());
        if (status.IsOk() || (status.Code() == common::Status::kNoData && outsize != 0)) {
          range.size = static_cast<u32>(outsize);
          range.pos  = 0;
          ranges_.push_back(std::move(range));
        }
        if (!status.IsOk() && (status.Code() != common::Status::kNoData)) {
          return std::make_tuple(status, 0);
        }
      }
    }

    typedef CmpPred<dir> Comp;
    typedef typename Comp::HeapItem HeapItem;
    typedef typename Comp::KeyType KeyType;
    typedef boost::heap::skew_heap<HeapItem, boost::heap::compare<Comp>> Heap;
    Heap heap;

    size_t index = 0;
    for(auto& range: ranges_) {
      if (!range.empty()) {
        KeyType key = range.top_key();
        heap.push({key, range.top(), index});
      }
      index++;
    }

    while(!heap.empty()) {
      HeapItem item = heap.top();
      size_t index = item.index;
      Sample const* sample = item.sample;
      if (size - outpos >= sample->payload.size) {
        memcpy(dest + outpos, sample, sample->payload.size);
        outpos += sample->payload.size;
      } else {
        // Output buffer is fully consumed
        return std::make_tuple(common::Status::Ok(), outpos);
      }
      heap.pop();
      ranges_[index].advance(sample->payload.size);
      if (ranges_[index].empty()) {
        // Refill range if possible
        common::Status status;
        size_t outsize;
        std::tie(status, outsize) = iters_[index]->read(ranges_[index].buffer.data(), ranges_[index].buffer.size());
        if (!status.IsOk() && (status.Code() != common::Status::kNoData)) {
          return std::make_tuple(status, 0);
        }
        ranges_[index].size = static_cast<u32>(outsize);
        ranges_[index].pos  = 0;
      }
      if (!ranges_[index].empty()) {
        KeyType point = ranges_[index].top_key();
        heap.push({point, ranges_[index].top(), index});
      }
    }
    if (heap.empty()) {
      iters_.clear();
      ranges_.clear();
    }
    // All iterators are fully consumed
    return std::make_tuple(common::Status::NoData(""), outpos);
  }
};

}  // namespace storage
}  // namespace faststdb

#endif  // FASTSTDB_STORAGE_OPERATORS_MERGE_H_
