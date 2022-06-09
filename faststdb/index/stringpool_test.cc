/*!
 * \file stringpool_test.cc
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include "faststdb/index/stringpool.h"

#include "gtest/gtest.h"

namespace faststdb {

TEST(TestStringPool, Test) {
  StringPool pool;
  const char* foo = "foo";
  auto id_foo = pool.add(foo, foo + 3);
  auto result_foo = pool.str(id_foo);
  const char* bar = "123456";
  auto id_bar = pool.add(bar, bar + 6);
  auto result_bar = pool.str(id_bar);
  EXPECT_STREQ(std::string(result_foo.first, result_foo.first + result_foo.second).c_str(), foo);
  EXPECT_STREQ(std::string(result_bar.first, result_bar.first + result_bar.second).c_str(), bar);
}

}  // namespace faststdb

