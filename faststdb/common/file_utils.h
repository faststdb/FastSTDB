/*!
 * \file file_utils.h
 */
#ifndef FASTSTDB_COMMON_FILE_UTILS_H_
#define FASTSTDB_COMMON_FILE_UTILS_H_

#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>

#include <fstream>
#include <string>

#include "faststdb/common/logging.h"

namespace faststdb {
namespace common {

inline std::string GetParentDir(const std::string& cur_path) {
  int i = cur_path.length() - 1;
  for (; i >= 0; --i) {
    if (cur_path[i] == '/') {
      break;
    }
  }
  auto len = i + 1;
  return cur_path.substr(0, len);
}

inline bool MakeDir(const std::string& dir_path) {
  if (access(dir_path.c_str(), 0) == 0) {
    return true;
  }
  auto ret = mkdir(dir_path.c_str(), 0755);
  if (ret != 0) {
    LOG(ERROR) << "create dir:" << dir_path << " error=" << strerror(errno);
    return false;
  } else {
    return true;
  }
}

inline void WriteFileContent(const std::string& file_path, const std::string& content) {
  std::ofstream os(file_path);
  os.write(content.c_str(), content.length());
  os.close();
}

inline void RemoveFile(const std::string& file_path) {
  remove(file_path.c_str());
}

}  // namespace common
}  // namespace faststdb

#endif  // FASTSTDB_COMMON_FILE_UTILS_H_
