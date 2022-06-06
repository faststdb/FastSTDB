/*!
 * \file meta_storage.h
 */
#ifndef FASTSTDB_STORAGE_META_STORAGE_H_
#define FASTSTDB_STORAGE_META_STORAGE_H_

#include <memory>
#include <mutex>
#include <thread>

#include "faststdb/common/blocking_queue.h"
#include "faststdb/common/singleton.h"
#include "faststdb/storage/volume_registry.h"
#include "faststdb/storage/meta_volume.pb.h"

namespace faststdb {
namespace storage {

class MetaVolumeStore {
 public:
  explicit MetaVolumeStore(const std::string& db_name);
  
  std::vector<VolumeRegistry::VolumeDesc> get_volumes();
  void add_volume(const VolumeRegistry::VolumeDesc& vol);
  void update_volume(const VolumeRegistry::VolumeDesc& vol);
  void flush();
  void Run();
  size_t PendingSize();
  void Exit();

 protected:
  proto::MetaVolume meta_volume_;
  std::mutex mutex_, flush_mutex_;
  std::shared_ptr<std::thread> async_thread_;
  common::BlockingQueue<VolumeRegistry::VolumeDesc> blocking_queue_;
};

class MetaVolumeStoreTable : public common::Singleton<MetaVolumeStoreTable> {
 public:
  std::shared_ptr<MetaVolumeStore> GetMetaVolume(
      const std::string& db_name);
  void Exit();

 protected:
  std::mutex mutex_;
  std::unordered_map<std::string, uint32_t> index_;
  std::vector<std::shared_ptr<MetaVolumeStore>> meta_volume_stores_;
};

class MetaStorage : public VolumeRegistry {
 public:
  explicit MetaStorage(const std::string& db_name);

  virtual ~MetaStorage();

  /** Read list of volumes and their sequence numbers.
   * @throw std::runtime_error in a case of error
   */
  std::vector<VolumeRegistry::VolumeDesc> get_volumes() override;

  /**
   * @brief Add NEW volume synchroniously
   * @param vol is a volume description
   */
  void add_volume(const VolumeRegistry::VolumeDesc& vol) override;

  /**
   * @brief Update volume metadata asynchronously
   * @param vol is a volume description
   */
  void update_volume(const VolumeRegistry::VolumeDesc& vol) override;

  /**
   * @brief Get name of the database
   * @return database name
   */
  std::string get_dbname() override;

  /***
   * @brief Get the async pending size
   * @return pending size.
   */
  size_t pending_size() override;

 protected:
  std::string db_name_;
  std::shared_ptr<MetaVolumeStore> meta_volume_store_;
};

}  // namespace storage
}  // namespace faststdb

#endif  // FASTSTDB_STORAGE_META_STORAGE_H_
