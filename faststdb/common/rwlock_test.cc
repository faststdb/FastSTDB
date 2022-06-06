/*!
 * \file rwlock_test.cc
 */
#include "faststdb/common/rwlock.h"

#include "gtest/gtest.h"

namespace faststdb {
namespace common {

TEST(TestRWLock, Test) {
  RWLock rwlock;

  rwlock.rdlock();
  rwlock.unlock();
}

}  // namespace common
}  // namespace faststdb
