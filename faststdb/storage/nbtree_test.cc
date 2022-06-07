/*!
 * \file nbtree_test.cc
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
#include "faststdb/storage/nbtree.h"

#include "gtest/gtest.h"

#include "faststdb/common/basic.h"
#include "faststdb/common/datetime.h"
#include "faststdb/common/file_utils.h"
#include "faststdb/common/timer.h"
#include "faststdb/storage/meta_storage.h"

namespace faststdb {
namespace storage {

void InitVolume() {
  static bool inited = false;
  if (inited) return;

  auto cfg_path = common::GetMetaVolumeDir() + "/db_nbtree";
  common::RemoveFile(cfg_path);

  std::shared_ptr<VolumeRegistry> volume_registry(new MetaStorage("db_nbtree"));
  auto meta_volume = MetaVolume::open_existing(volume_registry);

  u32 capacity = 1024 * 1024;
  meta_volume->add_volume(0, capacity, "/tmp/faststdb/db_nbtree_0.vol"); 
  Volume::create_new("/tmp/faststdb/db_nbtree_0.vol", capacity);

  inited = true;
}

TEST(TestNBTree, FixedSizeFileStorage) {
  InitVolume();

  std::shared_ptr<VolumeRegistry> volume_registry(new MetaStorage("db_nbtree"));
  auto blockstore = FixedSizeFileStorage::open(volume_registry);

  ParamId id = 0;
  std::vector<LogicAddr> logic_addr;
  std::shared_ptr<NBTreeExtentsList> nbtree_extent_list(new NBTreeExtentsList(id, logic_addr, blockstore));
  
  for (auto i = 0; i < 8; ++i) {
    Timestamp t = i;
    double val = 100 + i;
    auto append_result = nbtree_extent_list->append(t, val);
    EXPECT_EQ(NBTreeAppendResult::OK, append_result);
  }

  auto extents = nbtree_extent_list->get_extents();
  EXPECT_EQ(1, extents.size());

  extents[0]->debug_dump(std::cout, 2, [](Timestamp ts) { return DateTimeUtil::to_iso_string(ts); });
}

}  // namespace storage
}  // namespace faststdb

