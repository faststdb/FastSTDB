/*!
 * \file limiter.cc
 */
#include "faststdb/query/query_processing/limiter.h"

namespace faststdb {
namespace qp {

Limiter::Limiter(u64 limit, u64 offset, std::shared_ptr<Node> next)
    : limit_(limit)
      , offset_(offset)
      , counter_(0)
      , next_(next) { }

void Limiter::complete() {
  next_->complete();
}

bool Limiter::put(MutableSample &sample) {
  if (counter_ < offset_) {
    // continue iteration
    return true;
  } else if (counter_ >= limit_) {
    // stop iteration
    return false;
  }
  counter_++;
  return next_->put(sample);
}

void Limiter::set_error(common::Status status) {
  next_->set_error(status);
}

int Limiter::get_requirements() const {
  return TERMINAL;
}

}  // namespace qp
}  // namespace faststdb

