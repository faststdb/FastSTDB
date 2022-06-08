#pragma once

#include <memory>

#include "../queryprocessor_framework.h"

namespace faststdb {
namespace qp {

struct TopN : Node {

  struct Context {
    double last_xs;
    Timestamp last_ts;
    double sum;
    ParamId id;
  };

  std::unordered_map< ParamId
      , Context
      > table_;

  std::shared_ptr<Node> next_;

  size_t N_;

  TopN(size_t N, std::shared_ptr<Node> next);

  TopN(const boost::property_tree::ptree&, const ReshapeRequest&, std::shared_ptr<Node> next);

  virtual void complete();

  virtual bool put(MutableSample& sample);

  virtual void set_error(common::Status status);

  virtual int get_requirements() const;
};

}  // namespace qp
}  // namespace faststdb
