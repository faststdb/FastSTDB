/*!
 * \file limiter.h 
 */
#ifndef FASTSTDB_QUERY_QUERY_PROCESSING_LIMITER_H_
#define FASTSTDB_QUERY_QUERY_PROCESSING_LIMITER_H_

#include <memory>

#include "../queryprocessor_framework.h"

namespace faststdb {
namespace qp {

struct Limiter : Node {
  u64                   limit_;
  u64                   offset_;
  u64                   counter_;
  std::shared_ptr<Node> next_;

  Limiter(u64 limit, u64 offset, std::shared_ptr<Node> next);

  virtual void complete();

  virtual bool put(MutableSample& sample);

  virtual void set_error(common::Status status);

  virtual int get_requirements() const;
};

}  // namespace qp
}  // namespace faststdb

#endif  // FASTSTDB_QUERY_QUERY_PROCESSING_LIMITER_H_
