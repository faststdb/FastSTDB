/*!
 * \file crc32c_test.cc
 */
#include "faststdb/common/crc32c.h"

#include "gtest/gtest.h"

#include "faststdb/common/basic.h"
#include "faststdb/common/logging.h"

namespace faststdb {
namespace common {

TEST(crc32, test_crc32c_0) {
  auto crc32hw = chose_crc32c_implementation(CRC32C_hint::DETECT);
  auto crc32sw = chose_crc32c_implementation(CRC32C_hint::FORCE_SW);
  if (crc32hw == crc32sw) {
    LOG(ERROR) << "Can't compare crc32c implementation, hardware version is not available.";
    return;
  }
  auto gen = []() {
    return static_cast<u8>(rand());
  };
  std::vector<u8> data(111111, 0);
  std::generate(data.begin(), data.end(), gen);

  u32 hw = 0, sw = 0;
  hw = crc32hw(hw, data.data(), data.size());
  sw = crc32sw(sw, data.data(), data.size());

  EXPECT_EQ(hw, sw);
}

void test_crc32c_composability(CRC32C_hint hint) {
  auto crc32impl = chose_crc32c_implementation(hint);
  auto gen = []() {
    return static_cast<u8>(rand());
  };
  std::vector<u8> data(4096, 0);
  std::generate(data.begin(), data.end(), gen);

  u32 crc = 0;
  crc = crc32impl(crc, data.data(), 1024);
  crc = crc32impl(crc, data.data() + 1024, 1024);
  crc = crc32impl(crc, data.data() + 2048, 1024);
  crc = crc32impl(crc, data.data() + 3072, 1024);

  u32 expected_crc = 0;
  expected_crc = crc32impl(expected_crc, data.data(), data.size());

  EXPECT_EQ(expected_crc, crc);
}

TEST(crc32, test_crc32c_1) {
  test_crc32c_composability(CRC32C_hint::FORCE_SW);
}

TEST(crc32, test_crc32c_2) {
  test_crc32c_composability(CRC32C_hint::FORCE_HW);
}

}  // namespace common
}  // namespace faststdb
