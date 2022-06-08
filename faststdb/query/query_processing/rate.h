#pragma once

#include <memory>

#include "../queryprocessor_framework.h"

namespace faststdb {
namespace qp {

struct SimpleRate : Node {

  std::unordered_map< std::tuple<ParamId, u32>
      , std::tuple<Timestamp, double>
      , KeyHash
      , KeyEqual> table_;

  std::shared_ptr<Node> next_;

  SimpleRate(std::shared_ptr<Node> next);

  SimpleRate(const boost::property_tree::ptree&, const ReshapeRequest, std::shared_ptr<Node> next);

  virtual void complete();

  virtual bool put(MutableSample& sample);

  virtual void set_error(common::Status status);

  virtual int get_requirements() const;
};

struct CumulativeSum : Node {

  std::unordered_map< std::tuple<ParamId, u32>
      , double
      , KeyHash
      , KeyEqual> table_;

  std::shared_ptr<Node> next_;

  CumulativeSum(std::shared_ptr<Node> next);

  CumulativeSum(const boost::property_tree::ptree&, const ReshapeRequest&, std::shared_ptr<Node> next);

  virtual void complete();

  virtual bool put(MutableSample& sample);

  virtual void set_error(Status status);

  virtual int get_requirements() const;
};

}  // namespace qp
}  // namespace faststdb
