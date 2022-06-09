/*!
 * \file filterbyid.h
 */
#ifndef FASTSTDB_QUERY_QUERY_PROCESSING_FILTERBYID_H_ 
#define FASTSTDB_QUERY_QUERY_PROCESSING_FILTERBYID_H_

#include <memory>

#include "../queryprocessor_framework.h"

namespace faststdb {
namespace qp {

/** Filter ids using predicate.
 * Predicate is an unary functor that accepts parameter of type ParamId - fun(ParamId) -> bool.
 */
template <class Predicate>
struct FilterByIdNode : std::enable_shared_from_this<FilterByIdNode<Predicate>>, Node {
  //! Id matching predicate
  Predicate             op_;
  std::shared_ptr<Node> next_;

  FilterByIdNode(Predicate pred, std::shared_ptr<Node> next)
      : op_(pred)
        , next_(next) {}

  virtual void complete() { next_->complete(); }

  virtual bool put(MutableSample& sample) {
    return op_(sample.get_paramid()) ? next_->put(sample) : true;
  }

  void set_error(common::Status status) {
    if (!next_) {
      LOG(FATAL) << "bad query processor node, next not set";
    }
    next_->set_error(status);
  }

  virtual int get_requirements() const { return EMPTY; }
};

}  // namespace qp
}  // namespace faststdb

#endif  // FASTSTDB_QUERY_QUERY_PROCESSING_FILTERBYID_H_
