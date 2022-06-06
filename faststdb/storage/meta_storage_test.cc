/*!
 * \file meta_storage_test.cc
 */
#include "faststdb/storage/meta_storage.h"

#include "gtest/gtest.h"

#include "faststdb/common/basic.h"
#include "faststdb/common/file_utils.h"

namespace faststdb {
namespace storage {

TEST(TestMetaStorage, get_volumes) {
  auto cfg_path = common::GetMetaVolumeDir() + "/test.vol";
  common::RemoveFile(cfg_path);

  MetaStorage meta_storage("test.vol");
  auto volumes = meta_storage.get_volumes();
  EXPECT_EQ(0, volumes.size());
}

TEST(TestMetaStorage, add_volume) {
  MetaStorage meta_storage("test.vol");
  VolumeRegistry::VolumeDesc volume_desc;
  volume_desc.id = 1;
  volume_desc.capacity = 200;
  meta_storage.add_volume(volume_desc);
}

TEST(TestStorage, update_volume) {
  MetaStorage meta_storage("test.vol");
  auto volumes = meta_storage.get_volumes();
  EXPECT_EQ(1, volumes.size());
  EXPECT_EQ(200, volumes[0].capacity);

  VolumeRegistry::VolumeDesc volume_desc;
  volume_desc.id = 1;
  volume_desc.capacity = 300;
  meta_storage.update_volume(volume_desc);
}

}  // namespace storage
}  // namespace faststdb
