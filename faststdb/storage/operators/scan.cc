/*!
 * \file scan.cc
 */
#include "faststdb/storage/operators/scan.h"

namespace faststdb {
namespace storage {

std::tuple<common::Status, size_t> ChainOperator::read(Timestamp *destts, double *destval, size_t size) {
  common::Status status = common::Status::NoData("");
  size_t ressz = 0;  // current size
  size_t accsz = 0;  // accumulated size
  while(iter_index_ < iter_.size()) {
    std::tie(status, ressz) = iter_[iter_index_]->read(destts, destval, size);
    destts += ressz;
    destval += ressz;
    size -= ressz;
    accsz += ressz;
    if (size == 0) {
      break;
    }
    if (status.Code() == common::Status::kNoData ||
        status.Code() == common::Status::kUnavailable) {
      // this leaf node is empty or removed, continue with next
      iter_index_++;
      continue;
    }
    if (!status.IsOk()) {
      // Stop iteration or error!
      return std::tie(status, accsz);
    }
  }
  return std::tie(status, accsz);
}

RealValuedOperator::Direction ChainOperator::get_direction() {
  return dir_;
}


ChainMaterializer::ChainMaterializer(std::vector<ParamId>&& ids, std::vector<std::unique_ptr<RealValuedOperator>>&& it)
    : iters_(std::move(it))
    , ids_(std::move(ids))
    , pos_(0)
{
}

std::tuple<common::Status, size_t> ChainMaterializer::read(u8 *dest, size_t dest_size) {
  common::Status status = common::Status::NoData("");
  size_t ressz = 0;  // current size
  size_t accsz = 0;  // accumulated size
  size_t size = dest_size / sizeof(Sample);
  std::vector<Timestamp> destts_vec(size, 0);
  std::vector<double> destval_vec(size, 0);
  std::vector<ParamId> outids(size, 0);
  Timestamp* destts = destts_vec.data();
  double* destval = destval_vec.data();
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
    Sample* sample = reinterpret_cast<Sample*>(dest);
    dest += sizeof(Sample);
    sample->payload.type = PAYLOAD_FLOAT;
    sample->payload.size = sizeof(Sample);
    sample->paramid = outids[i];
    sample->timestamp = destts_vec[i];
    sample->payload.float64 = destval_vec[i];
  }
  return std::make_tuple(status, accsz*sizeof(Sample));
}

EventChainMaterializer::EventChainMaterializer(
    std::vector<ParamId>&& ids, std::vector<std::unique_ptr<BinaryDataOperator>>&& it)
    : iters_(std::move(it)), ids_(std::move(ids)), pos_(0), available_(false)
{
}

std::tuple<common::Status, size_t> EventChainMaterializer::read(u8 *dest, size_t dest_size) {
  common::Status status = common::Status::NoData("");
  size_t accsz = 0;  // accumulated size

  while (pos_ < iters_.size()) {
    if (!available_) {
      curr_id_ = ids_[pos_];
      size_t ressz = 0;
      std::tie(status, ressz) = iters_[pos_]->read(&curr_ts_, &curr_, 1);
      if (ressz == 0) {
        pos_++;
        continue;  // Try to fetch value from the next iterator
      }
      if (status.Code() != common::Status::kNoData) {
        if (!status.IsOk()) {
          // Stop iteration on error!
          break;
        }
      }
      available_ = true;
    }
    // Convert vectors to series of samples
    if (available_) {
      // Compute required space
      u32 space_needed = sizeof(Sample) + curr_.size();
      if (dest_size < space_needed) {
        break;
      }
      Sample sample = {};
      sample.payload.type = PAYLOAD_EVENT;
      sample.payload.size = static_cast<u16>(space_needed);  // guaranteed to fit u16
      sample.paramid = curr_id_;
      sample.timestamp = curr_ts_;
      sample.payload.float64 = 0;
      memcpy(dest, &sample, sizeof(sample));
      auto destsample = reinterpret_cast<Sample*>(dest);
      memcpy(destsample->payload.data, curr_.data(), curr_.size());
      dest += space_needed;
      dest_size -= space_needed;
      accsz += space_needed;
      available_ = false;
    }
  }
  return std::make_tuple(status, accsz);
}

}  // namespace storage
}  // namespace faststdb
