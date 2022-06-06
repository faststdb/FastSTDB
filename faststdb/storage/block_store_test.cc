/*!
 * \file block_store_test.cc
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
#include "faststdb/storage/block_store.h"

#include "gtest/gtest.h"

#include "faststdb/common/basic.h"
#include "faststdb/common/file_utils.h"
#include "faststdb/common/timer.h"
#include "faststdb/storage/meta_storage.h"

namespace faststdb {
namespace storage {

TEST(TestBlockStore, init_volume) {
  auto cfg_path = common::GetMetaVolumeDir() + "/db2";
  common::RemoveFile(cfg_path);

  std::shared_ptr<VolumeRegistry> volume_registry(new MetaStorage("db2"));
  auto meta_volume = MetaVolume::open_existing(volume_registry);

  u32 capacity = 1024 * 1024;
  meta_volume->add_volume(0, capacity, "/tmp/faststdb/db2_0.vol");
  Volume::create_new("/tmp/faststdb/db2_0.vol", capacity);
}

TEST(TestMetaVolume, FixedSizeFileStorage) {
  std::shared_ptr<VolumeRegistry> volume_registry(new MetaStorage("db2"));
  auto blockstore = ExpandableFileStorage::open(volume_registry);

  IOVecBlock io_vec_block;
  size_t number = 0;
  common::Timer timer;

  while (true) {
    common::Status status;
    LogicAddr logic_addr;
    std::tie(status, logic_addr) = blockstore->append_block(io_vec_block);

    if (!status.IsOk()) {
      LOG(INFO) << "Error:" << status.ToString();
      break;
    }
    if (number && ((number & 0xFF) == 0)) {
      LOG(INFO) << 1.0 / timer.elapsed() << "(MB/s)"; 
      LOG(INFO) << "pending size=" << volume_registry->pending_size();
      timer.restart();
    }
    if (number && ((number & 0xFFF) == 0)) {
      blockstore->flush();
      break;
    }
    ++number;
  }
}

}  // namespace storage
}  // namespace faststdb

