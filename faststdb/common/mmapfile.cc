/*!
 * \file mmapfile.cc
 */
#include "faststdb/common/mmapfile.h"

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/mman.h>
#include <iostream>

#include "faststdb/common/logging.h"

namespace faststdb {
namespace common {

MMapFile::MMapFile() : mmap_size_(0) { }
 
MMapFile::~MMapFile() {
  if (fd_ >= 0 && base_) {
    auto ret = ftruncate(fd_, mmap_size_);
    if (ret != 0) {
      LOG(ERROR) << "failed ftruncate to mmap file["
          << file_name_ << "], emsg: "
          << strerror(errno);
    }
    ret = munmap(base_, mmap_size_);
    if (ret != 0) {
      LOG(ERROR) << "failed to munmap file["
          << file_name_ << "], emsg: "
          << strerror(errno);
    }
    ::close(fd_);
  }
  fd_ = -1;
  base_ = NULL;
  return;
}

Status MMapFile::Init(const char* fileName, uint64_t capacity) {
  file_name_ = fileName;

  do {
    auto ret = access(file_name_.c_str(), F_OK);
    if (ret == 0) {
      if (LoadFile() < 0) {
        break;
      }
    } else if (ret == -1 && errno == ENOENT && capacity != 0) {
      if (CreateFile(capacity) < 0) {
        break;
      }
    } else {
      break;
    }
    return Status::Ok();
  } while (0);

  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
  if (base_) {
    if (MAP_FAILED != base_) {
      munmap(base_, mmap_size_);
    }
    base_ = NULL;
  }
  return Status::Internal(
      std::string("Load or Create File:") + fileName + " failed");
}

int32_t MMapFile::LoadFile() {
  int openFlags = O_RDWR;
  int mmapProt = PROT_READ | PROT_WRITE;

  // open
  fd_ = open(file_name_.c_str(), openFlags);
  if (fd_ < 0) {
    LOG(ERROR) << "failed to open file[" << file_name_ << "]";
    return -1;
  }
  struct stat statbuf;
  fstat(fd_, &statbuf);
  mmap_size_ = statbuf.st_size; 

  // mapping
  base_ = reinterpret_cast<char*>(mmap(NULL, mmap_size_, mmapProt,
                                       MAP_SHARED, fd_, 0));
  if (MAP_FAILED == base_) {
    LOG(ERROR) << "failed to mmap file[" << file_name_ << "], emsg:" << strerror(errno);
    return -1;
  }
  return 0;
}

int32_t MMapFile::CreateFile(uint64_t capacity) {
  mmap_size_ = capacity;

  // open
  fd_ = open(file_name_.c_str(), O_RDWR | O_CREAT, S_IRWXU | S_IRGRP | S_IROTH);
  if (fd_ < 0) {
    LOG(ERROR) << "failed to open " << file_name_;
    return -1;
  }

  // mapping
  base_ = reinterpret_cast<char*>(mmap(NULL, mmap_size_,
                                       PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0));
  if (MAP_FAILED == base_) {
    LOG(ERROR) << "faild to mmap file[" << file_name_ << "], mmap_size_:" << mmap_size_
        << ", emsg:" << strerror(errno);
    return -1;
  }

  return 0;
}

}  // namespace common
}  // namespace faststdb
