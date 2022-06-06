/*!
 * \file status_test.cc
 */
#include "faststdb/common/status.h"

#include "gtest/gtest.h"

namespace faststdb {
namespace common {

TEST(TestStatus, IsOk) {
  Status status;
  EXPECT_TRUE(status.IsOk());
}

}  // namespace common
}  // namespace faststdb
