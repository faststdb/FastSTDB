/*!
 * \file meta_storage.cc
 */
#include "faststdb/storage/meta_storage.h"

#include "faststdb/common/basic.h"
#include "faststdb/common/file_utils.h"
#include "faststdb/common/proto_configure.h"

namespace faststdb {
namespace storage {

MetaVolumeStore::MetaVolumeStore(const std::string& db_name) {
  common::MakeDir(common::GetMetaVolumeDir());
  auto cfg_path = common::GetMetaVolumeDir() + "/" + db_name;
  common::ProtoConfigure proto_configure;
  auto status = proto_configure.Init("faststdb.storage.proto.MetaVolume", cfg_path);
  if (status == common::ProtoConfigure::kOK) {
    meta_volume_ = *(dynamic_cast<const proto::MetaVolume*>(proto_configure.config()));
  } else {
    meta_volume_.set_db_name(db_name);
  }
  async_thread_.reset(new std::thread([this]() { this->Run(); }));
  async_thread_->detach();
}

std::vector<VolumeRegistry::VolumeDesc> MetaVolumeStore::get_volumes() {
  std::lock_guard<std::mutex> lck(mutex_);
  std::vector<VolumeRegistry::VolumeDesc> volume_descs;
  volume_descs.reserve(meta_volume_.volume_desc_size());

  for (auto& item : meta_volume_.volume_desc()) {
    VolumeRegistry::VolumeDesc volume_desc;
    volume_desc.id = item.id();
    volume_desc.path = item.path();
    volume_desc.version = item.version();
    volume_desc.nblocks = item.nblocks();
    volume_desc.capacity = item.capacity();
    volume_desc.generation = item.generation();
    volume_descs.push_back(volume_desc);
  }
  return volume_descs;
}

void MetaVolumeStore::add_volume(const VolumeRegistry::VolumeDesc& vol) {
  {
    std::lock_guard<std::mutex> lck(mutex_);
    auto item = meta_volume_.add_volume_desc();
    item->set_id(vol.id);
    item->set_path(vol.path);
    item->set_version(vol.version);
    item->set_nblocks(vol.nblocks);
    item->set_capacity(vol.capacity);
    item->set_generation(vol.generation);
  }

  flush();
}

void MetaVolumeStore::update_volume(const VolumeRegistry::VolumeDesc& vol) {
  blocking_queue_.Push(vol);
}

void MetaVolumeStore::flush() {
  auto content = meta_volume_.DebugString();
  auto cfg_path = common::GetMetaVolumeDir() + "/" + meta_volume_.db_name();
  std::lock_guard<std::mutex> lck(flush_mutex_); 
  common::WriteFileContent(cfg_path, content);
}

void MetaVolumeStore::Run() {
  std::vector<VolumeRegistry::VolumeDesc> vols;
  while (blocking_queue_.Pop(vols)) {
    {
      std::lock_guard<std::mutex> lck(mutex_);
      for (auto& vol : vols) {
        auto id = vol.id;
        auto item = meta_volume_.mutable_volume_desc(id);
        item->set_path(vol.path);
        item->set_version(vol.version);
        item->set_nblocks(vol.nblocks);
        item->set_capacity(vol.capacity);
        item->set_generation(vol.generation);
      }
    }
    flush();
  }
}

size_t MetaVolumeStore::PendingSize() {
  return blocking_queue_.Size();
}

void MetaVolumeStore::Exit() {
  blocking_queue_.Exit();
}

std::shared_ptr<MetaVolumeStore> MetaVolumeStoreTable::GetMetaVolume(const std::string& db_name) {
  std::lock_guard<std::mutex> lck(mutex_);
  auto iter = index_.find(db_name);
  if (iter != index_.end()) {
    return meta_volume_stores_[iter->second];
  } else {
    std::shared_ptr<MetaVolumeStore> meta_volume(new MetaVolumeStore(db_name));
    index_[db_name] = meta_volume_stores_.size();
    meta_volume_stores_.emplace_back(meta_volume);
    return meta_volume;
  }
}

void MetaVolumeStoreTable::Exit() {
  for (auto meta_volume_store : meta_volume_stores_) {
    meta_volume_store->Exit();
  }
}

MetaStorage::MetaStorage(const std::string& db_name) : db_name_(db_name) {
  meta_volume_store_ = MetaVolumeStoreTable::Get()->GetMetaVolume(db_name_);
}

MetaStorage::~MetaStorage() {
  MetaVolumeStoreTable::Get()->Exit();
}

std::vector<VolumeRegistry::VolumeDesc> MetaStorage::get_volumes() {
  return meta_volume_store_->get_volumes();
}

void MetaStorage::add_volume(const VolumeRegistry::VolumeDesc& vol) {
  return meta_volume_store_->add_volume(vol);
}

void MetaStorage::update_volume(const VolumeRegistry::VolumeDesc& vol) {
  meta_volume_store_->update_volume(vol);
}

std::string MetaStorage::get_dbname() {
  return db_name_;
}

size_t MetaStorage::pending_size() {
  return meta_volume_store_->PendingSize();
}

}  // namespace storage
}  // namespace faststdb
