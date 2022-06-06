/*!
 * \file mmapfile.h
 */
#ifndef FASTSTDB_COMMON_MMAPFILE_H_
#define FASTSTDB_COMMON_MMAPFILE_H_

#include <stdint.h>
#include <string>
#include <string.h>

#include <mutex>

#include "faststdb/common/status.h"

namespace faststdb {
namespace common {

class MMapFile  {
 public:
  MMapFile();
  ~MMapFile();

  Status Init(const char* fileName, uint64_t capacity = 0);

  inline char* GetAddress(const int64_t& offset) {
    return base_ + offset;
  }
  inline char* GetBase() { return base_; }
  inline uint64_t GetMmapSize() { return mmap_size_; }
  inline int32_t fd() const { return fd_; }

 protected:
  int32_t LoadFile();
  int32_t CreateFile(uint64_t capacity);

 protected:
  std::string file_name_;
  int32_t fd_ = -1;
  char* base_ = NULL;

  uint64_t mmap_size_ = 0;
};

}  // namespace common
}  // namespace faststdb

#endif  // FASTSTDB_COMMON_MMAPFILE_H_
