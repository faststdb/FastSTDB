/*!
 * \file queryprocessor_framework_test.cc
 */
#include "faststdb/query/queryprocessor_framework.h"

#include "gtest/gtest.h"

namespace faststdb {
namespace qp {

TEST(TestQuery, Test_1) {
  auto items = list_query_registry();
  for (auto& item : items) {
    LOG(INFO) << "item=" << item;
  }
}

}  // namespace qp
}  // namespace faststdb
