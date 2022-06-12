/*!
 * \file eval_test.cc
 */
#include "faststdb/query/query_processing/eval.h"

#include <algorithm>
#include <iostream>
#include <random>

#include "gtest/gtest.h"

#include "faststdb/query/queryprocessor_framework.h"
#include <boost/property_tree/json_parser.hpp>

namespace faststdb {
namespace qp {

struct MockNode : Node {
  common::Status status_;
  double result_;

  MockNode()
      : status_(common::Status::Ok())
        , result_(0)
  {}

  void complete() {}
  bool put(MutableSample &sample) {
    result_ = *sample[0];
    return true;
  }
  void set_error(common::Status status) { status_ = status; }
  int get_requirements() const {
    return 0;
  }
};

struct BigSample : Sample {
  char pad[1024];
};

void init_request(ReshapeRequest *req) {
  // Build series names
  std::vector<std::string> names = {
    "col0 foo=bar",
    "col1 foo=bar",
    "col2 foo=bar",
    "col3 foo=bar",
    "col4 foo=bar",
    "col5 foo=bar",
    "col6 foo=bar",
    "col7 foo=bar",
    "col8 foo=bar",
    "col9 foo=bar",
  };
  std::vector<ParamId> ids = {
    1000,
    1001,
    1002,
    1003,
    1004,
    1005,
    1006,
    1007,
    1008,
    1009,
  };
  req->select.columns.resize(ids.size());
  req->select.matcher.reset(new PlainSeriesMatcher());
  for (u32 i = 0; i < ids.size(); i++) {
    req->select.columns.at(i).ids.push_back(ids[i]);
    req->select.matcher->_add(names[i], static_cast<i64>(ids[i]));
  }
  req->select.global_matcher = req->select.matcher.get();
}

void init_sample(Sample& src, std::initializer_list<double>&& list) {
  src = {};
  src.paramid = 42;
  src.timestamp = 112233;
  if (list.size() == 1) {
    src.payload.type = PAYLOAD_FLOAT;
    src.payload.size = sizeof(Sample);
    src.payload.float64 = *list.begin();
  } else {
    char* dest = src.payload.data;
    int cnt = 0;
    for (auto it = list.begin(); it != list.end(); it++) {
      double value = *it;
      memcpy(dest, &value, sizeof(value));
      dest += sizeof(value);
      cnt++;
    }
    u64 mask = (1 << cnt) - 1;
    memcpy(&src.payload.float64, &mask, sizeof(mask));
    src.payload.size = sizeof(Sample) + sizeof(double) * cnt;
    src.payload.type = PAYLOAD_TUPLE;
  }
}

boost::property_tree::ptree init_ptree(const char* tc) {
  std::stringstream json;
  json << tc;
  boost::property_tree::ptree ptree;
  boost::property_tree::json_parser::read_json(json, ptree);
  return ptree;
}

TEST(ExprEval, Test_expr_eval_1) {
  ReshapeRequest req;
  init_request(&req);
  auto ptree = init_ptree("{\"expr\":\"1 + 2 + 3 + 4\"}");
  auto next = std::make_shared<MockNode>();
  ExprEval eval(ptree, req, next);
  Sample src;
  init_sample(src, { 11 });
  MutableSample ms(&src);
  eval.put(ms);
  EXPECT_EQ(10, next->result_);
}

TEST(ExprEval, Test_expr_eval_2) {
  ReshapeRequest req;
  init_request(&req);
  auto ptree = init_ptree("{\"expr\":\"1 + 2 + 3 + col0 + col1\"}");
  auto next = std::make_shared<MockNode>();
  ExprEval eval(ptree, req, next);
  BigSample src;
  init_sample(src, { 4, 5 });
  MutableSample ms(&src);
  eval.put(ms);
  EXPECT_EQ(15, next->result_);
}

TEST(TestEval, Test_expr_eval_3) {
  // Multiple expressions
  ReshapeRequest req;
  init_request(&req);
  auto ptree = init_ptree("{\"expr\":\"1, 2\"}");
  auto next = std::make_shared<MockNode>();
  try {
    ExprEval eval(ptree, req, next);
  } catch (...) {
    EXPECT_TRUE(true);
    return;
  }
  EXPECT_TRUE(false);
}

TEST(TestEval, Test_expr_eval_4) {
  // Unknown metric names
  ReshapeRequest req;
  init_request(&req);
  auto ptree = init_ptree("{\"expr\":\"1 + foo + bar\"}");
  auto next = std::make_shared<MockNode>();
  try {
    ExprEval eval(ptree, req, next);
  } catch (...) {
    EXPECT_TRUE(true);
    return;
  }
  EXPECT_TRUE(false);
}

TEST(TestEval, Test_expr_eval_5) {
  // Invalid expression
  ReshapeRequest req;
  init_request(&req);
  auto ptree = init_ptree("{\"expr\":\"1 x 1\"}");
  auto next = std::make_shared<MockNode>();
  try {
    ExprEval eval(ptree, req, next);
  } catch (...) {
    EXPECT_TRUE(true);
    return;
  }
  EXPECT_TRUE(false);
}

}  // namespace qp
}  // namespace faststdb
