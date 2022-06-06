/*!
 * \file volume_test.cc
 */
#include "faststdb/storage/volume.h"

#include "gtest/gtest.h"

#include "faststdb/common/basic.h"
#include "faststdb/common/file_utils.h"
#include "faststdb/storage/meta_storage.h"

namespace faststdb {
namespace storage {

TEST(TestMetaVolume, get_nvolumes) {
  auto cfg_path = common::GetMetaVolumeDir() + "/db1";
  common::RemoveFile(cfg_path);
  
  std::shared_ptr<VolumeRegistry> volume_registry(new MetaStorage("db1"));
  auto meta_volume = MetaVolume::open_existing(volume_registry);
  EXPECT_EQ(0, meta_volume->get_nvolumes());
}

TEST(TestMetaVolume, add_volume) {
  std::shared_ptr<VolumeRegistry> volume_registry(new MetaStorage("db1"));
  auto meta_volume = MetaVolume::open_existing(volume_registry);
  meta_volume->add_volume(0, 1024, "/tmp/volume1");
  meta_volume->add_volume(1, 1024, "/tmp/volume2"); 
  EXPECT_EQ(2, meta_volume->get_nvolumes());
}

TEST(TestMetaVolume, update_volume) {
  std::shared_ptr<VolumeRegistry> volume_registry(new MetaStorage("db1"));
  auto meta_volume = MetaVolume::open_existing(volume_registry);

  u32 nblocks = 0, capacity = 0;
  common::Status status;
  std::tie(status, nblocks) = meta_volume->get_nblocks(1);
  EXPECT_TRUE(status.IsOk());
  EXPECT_EQ(0, nblocks);

  std::tie(status, capacity) = meta_volume->get_capacity(1);
  EXPECT_TRUE(status.IsOk());
  EXPECT_EQ(1024, capacity);
}

}  // namespace storage
}  // namespace faststdb

