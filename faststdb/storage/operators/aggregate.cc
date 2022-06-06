/*!
 * \file aggregate.cc
 */
#include "faststdb/storage/operators/aggregate.h"
#include "faststdb/storage/tuples.h"

#include <cassert>

namespace faststdb {
namespace storage {

void CombineAggregateOperator::add(std::unique_ptr<AggregateOperator>&& it) {
  iter_.push_back(std::move(it));
}

std::tuple<common::Status, size_t> CombineAggregateOperator::read(Timestamp *destts, AggregationResult *destval, size_t size) {
  if (size == 0) {
    return std::make_tuple(common::Status::BadArg(""), 0);
  }
  if (iter_index_ == iter_.size()) {
    return std::make_tuple(common::Status::NoData(""), 0);
  }
  const size_t SZBUF = 1024;
  common::Status status = common::Status::NoData("");
  AggregationResult xsresult = INIT_AGGRES;
  Timestamp tsresult = 0;
  std::vector<AggregationResult> outval(SZBUF, INIT_AGGRES);
  std::vector<Timestamp> outts(SZBUF, 0);
  ssize_t ressz;
  int nagg = 0;
  while(iter_index_ < iter_.size()) {
    std::tie(status, ressz) = iter_[iter_index_]->read(outts.data(), outval.data(), SZBUF);
    if (ressz != 0) {
      xsresult = std::accumulate(outval.begin(), outval.begin() + ressz, xsresult,
                                 [](AggregationResult lhs, AggregationResult rhs) {
                                   lhs.combine(rhs);
                                   return lhs;
                                 });
      tsresult = outts[static_cast<size_t>(ressz)-1];
      nagg++;
    }
    if (status.Code() == common::Status::kNoData) {
      // This leaf node is empty, continue with next.
      iter_index_++;
      continue;
    }
    if (!status.IsOk()) {
      // Failure, stop iteration.
      return std::make_pair(status, 0);
    }
  }
  size_t result_size = 0;
  if (nagg != 0) {
    result_size = 1;
    destts[0] = tsresult;
    destval[0] = xsresult;
  }
  return std::make_tuple(common::Status::Ok(), result_size);
}

AggregateOperator::Direction CombineAggregateOperator::get_direction() {
  return dir_;
}

void FanInAggregateOperator::add(std::unique_ptr<AggregateOperator>&& it) {
  iter_.push_back(std::move(it));
}

std::tuple<common::Status, size_t> FanInAggregateOperator::read(Timestamp *destts, AggregationResult *destval, size_t size) {
  static const Timestamp MAX_TS = std::numeric_limits<Timestamp>::max();
  if (size == 0) {
    return std::make_tuple(common::Status::BadArg(""), 0);
  }
  if (iter_index_ == iter_.size()) {
    return std::make_tuple(common::Status::NoData(""), 0);
  }
  const size_t SZBUF = iter_.size();
  common::Status status = common::Status::NoData("");
  std::vector<AggregationResult> outval(SZBUF, INIT_AGGRES);
  std::vector<Timestamp> outts(SZBUF, MAX_TS);
  ssize_t ressz;
  size_t ixout = 0;
  Timestamp tsmin = MAX_TS;
  while (ixout < size) {
    int ix = 0;
    u32 nz = 0;
    for (auto& it: iter_) {
      // On the first iteration all values will match. On any other iteration
      // only the ones that needs to be updated will match.
      if (outts[ix] == tsmin) {
        std::tie(status, ressz) = it->read(&outts[ix], &outval[ix], 1);
        if (ressz == 0) {
          outval[ix] = INIT_AGGRES;
          outts[ix] = MAX_TS;
          nz++;
        }
        if (!status.IsOk() && (status.Code() != common::Status::kNoData)) {
          return std::make_pair(status, 0);
        }
        if (nz == outval.size()) {
          return std::make_pair(common::Status::NoData(""), ixout);
        }
      }
      ix++;
    }
    tsmin = std::accumulate(outts.begin(), outts.end(), MAX_TS,
                            [](Timestamp lhs, Timestamp rhs) {
                              return std::min(lhs, rhs);
                            });
    if (tsmin == MAX_TS) {
      return std::make_pair(common::Status::NoData(""), ixout);
    }
    AggregationResult xsresult = INIT_AGGRES;
    for (u32 i = 0; i < iter_.size(); i++) {
      // Invariant: at least one index matches by definition
      if (outts[i] == tsmin) {
        xsresult.combine(outval[i]);
      }
    }
    destval[ixout] = xsresult;
    destts [ixout] = tsmin;
    ixout++;
  }
  return std::make_tuple(common::Status::Ok(), ixout);
}

AggregateOperator::Direction FanInAggregateOperator::get_direction() {
  return dir_;
}

bool CombineGroupAggregateOperator::can_read() const {
  return rdpos_ < rdbuf_.size();
}

u32 CombineGroupAggregateOperator::elements_in_rdbuf() const {
  return static_cast<u32>(rdbuf_.size()) - rdpos_;  // Safe to cast because rdbuf_.size() <= RDBUF_SIZE
}

std::tuple<common::Status, size_t> CombineGroupAggregateOperator::copy_to(Timestamp* desttx, AggregationResult* destxs, size_t size) {
  common::Status status = common::Status::Ok();
  size_t copied = 0;
  while (status.IsOk() && size > 0) {
    size_t n = elements_in_rdbuf();
    if (iter_index_ != iter_.size()) {
      if (n < 2) {
        status = refill_read_buffer();
        continue;
      }
      // We can copy last element of the rdbuf_ to the output only if all
      // iterators were consumed! Otherwise invariant will be broken.
      n--;
    } else {
      if (n == 0) {
        break;
      }
    }

    auto tocopy = std::min(n, size);

    // Copy elements
    for (size_t i = 0; i < tocopy; i++) {
      auto const& bottom = rdbuf_.at(rdpos_);
      rdpos_++;
      const bool forward = dir_ == Direction::FORWARD;
      Timestamp bin = forward ? (bottom._begin - begin_) / step_
          : (begin_ - bottom._begin) / step_;
      *desttx++ = forward ? begin_ + (step_ * bin) : begin_ - (step_ * bin);
      *destxs++ = bottom;
      size--;
    }
    copied += tocopy;
  }
  return std::make_tuple(status, copied);
}


common::Status CombineGroupAggregateOperator::refill_read_buffer() {
  common::Status status = common::Status::NoData("");
  if (iter_index_ == iter_.size()) {
    return common::Status::NoData("");
  }

  u32 pos = 0;
  if (!rdbuf_.empty()) {
    auto tail = rdbuf_.back();  // the last element should be saved because it is possible that
    // it's not full (part of the range contained in first iterator
    // and another part in second iterator or even in more than one
    // iterators).
    rdbuf_.clear();
    rdbuf_.resize(RDBUF_SIZE, INIT_AGGRES);
    rdpos_ = 0;
    rdbuf_.at(0) = tail;
    pos = 1;
  } else {
    rdbuf_.clear();
    rdbuf_.resize(RDBUF_SIZE, INIT_AGGRES);
    rdpos_ = 0;
  }

  while(iter_index_ < iter_.size()) {
    size_t size = rdbuf_.size() - pos;
    if (size == 0) {
      break;
    }
    // NOTE: We can't read data from iterator directly to the rdbuf_ because buckets
    //       can be split between two iterators. In this case we should join such
    //       values together.
    // Invariant: rdbuf_ have enough room to fit outxs in the worst case (the worst
    //       case: `read` returns `size` elements and ranges isn't overlapping and we
    //       don't need to merge last element of `rdbuf_` with first elem of `outxs`).
    std::vector<AggregationResult> outxs(size, INIT_AGGRES);
    std::vector<Timestamp>           outts(size, 0);
    u32 outsz;
    std::tie(status, outsz) = iter_[iter_index_]->read(outts.data(), outxs.data(), outxs.size());
    if (outsz != 0) {
      if (pos > 0) {
        // Check last and first values of rdbuf_ and outxs
        auto const& last  = rdbuf_.at(pos - 1);
        auto const& first = outxs.front();
        Timestamp lastts  = dir_ == Direction::FORWARD ? last._begin - begin_
            : begin_ - last._begin;
        Timestamp firstts = dir_ == Direction::FORWARD ? first._begin - begin_
            : begin_ - first._begin;
        auto lastbin = lastts / step_;
        auto firstbin = firstts / step_;

        if (lastbin == firstbin) {
          pos--;
        }
      }
    }
    for (size_t ix = 0; ix < outsz; ix++) {
      rdbuf_.at(pos).combine(outxs.at(ix));
      const auto newdelta = rdbuf_.at(pos)._end - rdbuf_.at(pos)._begin;
      if (newdelta > step_) {
        assert(newdelta <= step_);
      }
      pos++;
    }
    if (status.Code() == common::Status::kNoData) {
      // This leaf node is empty, continue with next.
      iter_index_++;
      continue;
    }
    if (!status.IsOk()) {
      // Failure, stop iteration.
      rdbuf_.resize(pos);
      return status;
    }
  }
  rdbuf_.resize(pos);
  return common::Status::Ok();
}

std::tuple<common::Status, size_t> CombineGroupAggregateOperator::read(Timestamp *destts, AggregationResult *destval, size_t size) {
  if (size == 0) {
    return std::make_tuple(common::Status::BadArg(""), 0);
  }
  return copy_to(destts, destval, size);
}

CombineGroupAggregateOperator::Direction CombineGroupAggregateOperator::get_direction() {
  return dir_;
}

AggregateMaterializer::AggregateMaterializer(
    std::vector<ParamId>&& ids,
    std::vector<std::unique_ptr<AggregateOperator>>&& it,
    std::vector<AggregationFunction> &&func)
    : iters_(std::move(it))
    , ids_(std::move(ids))
    , pos_(0)
    , func_(std::move(func)) { }

std::tuple<common::Status, size_t> AggregateMaterializer::read(u8* dest, size_t size) {
  common::Status status = common::Status::NoData("");
  size_t nelements = 0;
  while(pos_ < iters_.size()) {
    Timestamp destts = 0;
    AggregationResult destval;
    size_t outsz = 0;
    std::tie(status, outsz) = iters_[pos_]->read(&destts, &destval, size);
    if (outsz == 0 && (status.Code() == common::Status::kNoData)) {
      // Move to next iterator
      pos_++;
      continue;
    }
    if (outsz != 1) {
      LOG(INFO) << "Unexpected aggregate size " << outsz;
      continue;
    }
    // create sample
    Sample sample;
    sample.paramid = ids_.at(pos_);
    sample.payload.type = PAYLOAD_FLOAT;
    sample.payload.size = sizeof(Sample);
    auto fun = func_.at(pos_);
    switch (fun) {
      case AggregationFunction::MIN:
        sample.timestamp = destval.mints;
        sample.payload.float64 = destval.min;
        break;
      case AggregationFunction::MIN_TIMESTAMP:
        sample.timestamp = destval.mints;
        sample.payload.float64 = destval.mints;
        break;
      case AggregationFunction::MAX:
        sample.timestamp = destval.maxts;
        sample.payload.float64 = destval.max;
        break;
      case AggregationFunction::MAX_TIMESTAMP:
        sample.timestamp = destval.maxts;
        sample.payload.float64 = destval.maxts;
        break;
      case AggregationFunction::SUM:
        sample.timestamp = destval._end;
        sample.payload.float64 = destval.sum;
        break;
      case AggregationFunction::CNT:
        sample.timestamp = destval._end;
        sample.payload.float64 = destval.cnt;
        break;
      case AggregationFunction::MEAN:
        sample.timestamp = destval._end;
        sample.payload.float64 = destval.sum/destval.cnt;
        break;
      case AggregationFunction::LAST:
        sample.timestamp = destval._end;
        sample.payload.float64 = destval.last;
        break;
      case AggregationFunction::FIRST:
        sample.timestamp = destval._begin;
        sample.payload.float64 = destval.first;
        break;
      case AggregationFunction::LAST_TIMESTAMP:
        sample.timestamp = destval._end;
        sample.payload.type = PAYLOAD_NONE;
        break;
      case AggregationFunction::FIRST_TIMESTAMP:
        sample.timestamp = destval._begin;
        sample.payload.type = PAYLOAD_NONE;
        break;
    }
    memcpy(dest, &sample, sizeof(sample));
    // move to next
    nelements += 1;
    size -= sizeof(sample);
    dest += sizeof(sample);
    pos_++;
    if (size < sizeof(sample)) {
      break;
    }
    if (status.Code() == common::Status::kNoData) {
      // this iterator is done, continue with next
      continue;
    }
    if (!status.IsOk()) {
      // Stop iteration on error!
      break;
    }
  }
  return std::make_tuple(status, nelements * sizeof(Sample));
}


std::tuple<common::Status, size_t> SeriesOrderAggregateMaterializer::read(u8 *dest, size_t dest_size) {
  common::Status status = common::Status::NoData("");
  size_t ressz = 0;  // current size
  size_t accsz = 0;  // accumulated size
  size_t sample_size = get_tuple_size(tuple_);
  size_t size = dest_size / sample_size;
  std::vector<Timestamp> destts_vec(size, 0);
  std::vector<AggregationResult> destval_vec(size, INIT_AGGRES);
  std::vector<ParamId> outids(size, 0);
  Timestamp* destts = destts_vec.data();
  AggregationResult* destval = destval_vec.data();
  while(pos_ < iters_.size()) {
    ParamId curr = ids_[pos_];
    std::tie(status, ressz) = iters_[pos_]->read(destts, destval, size);
    for (size_t i = accsz; i < accsz+ressz; i++) {
      outids[i] = curr;
    }
    destts += ressz;
    destval += ressz;
    size -= ressz;
    accsz += ressz;
    if (size == 0) {
      break;
    }
    pos_++;
    if (status.Code() == common::Status::kNoData) {
      // this iterator is done, continue with next
      continue;
    }
    if (!status.IsOk()) {
      // Stop iteration on error!
      break;
    }
  }
  // Convert vectors to series of samples
  for (size_t i = 0; i < accsz; i++) {
    double* tup;
    Sample* sample;
    std::tie(sample, tup)   = cast(dest);
    dest                   += sample_size;
    sample->payload.type    = PAYLOAD_TUPLE | PData::REGULLAR;
    sample->payload.size    = static_cast<u16>(sample_size);
    sample->paramid         = outids[i];
    sample->timestamp       = destts_vec[i];
    sample->payload.float64 = get_flags(tuple_);
    set_tuple(tup, tuple_, destval_vec[i]);
  }
  return std::make_tuple(status, accsz * sample_size);
}

}  // namespace storage
}  // namespace faststdb
