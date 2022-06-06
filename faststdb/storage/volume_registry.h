/*!
 * \file volume_registry.h
 */
#ifndef FASTSTDB_STORAGE_VOLUME_REGISTRY_H_
#define FASTSTDB_STORAGE_VOLUME_REGISTRY_H_

#include <vector>
#include <string>

#include "faststdb/common/basic.h"

namespace faststdb {
namespace storage {

/**
 * @brief Volume manager interface
 */
struct VolumeRegistry {
  virtual ~VolumeRegistry() { }

  typedef struct {
    u32 id;
    std::string path;
    u32 version;
    u32 nblocks;
    u32 capacity;
    u32 generation;
  } VolumeDesc;

  /** Read list of volumes and their sequence numbers.
   * @throw std::runtime_error in a case of error
   */
  virtual std::vector<VolumeDesc> get_volumes() = 0;

  /**
   * @brief Add NEW volume synchroniously
   * @param vol is a volume description
   */
  virtual void add_volume(const VolumeDesc& vol) = 0;


  /**
   * @brief Update volume metadata asynchronously
   * @param vol is a volume description
   */
  virtual void update_volume(const VolumeDesc& vol) = 0;

  /**
   * @brief Get name of the database
   * @return database name
   */
  virtual std::string get_dbname() = 0;

  /**
   * @brief Get the async pending size
   * @return the size.
   */
  virtual size_t pending_size() = 0;
};

}  // namespace storage
}  // namespace faststdb

#endif  // FASTSTDB_STORAGE_VOLUME_REGISTRY_H_
