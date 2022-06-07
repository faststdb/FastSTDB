/*!
 * \file status.h
 */
#ifndef FASTSTDB_COMMON_STATUS_H_
#define FASTSTDB_COMMON_STATUS_H_

#include <memory>
#include <string>
#include <algorithm>

#include "faststdb/common/thread_local.h"

namespace faststdb {
namespace common {

class Status {
 public:
  // In FastSTDB, status can not cross threads.
  typedef ThreadLocalStore<std::string> ErrorDetailString;

  enum ErrorCode {
    kOk = 0,
    kOutOfRange,
    kOverflow,
    kFileSeekError,
    kFileWriteError,
    kBadArg,
    kBadData,
    kNoData,
    kUnavailable,
    kNotFound,
    kNoMemory,
    kEAccess,
    kInternal,
    kUnknown
  };

  Status() : code_(kOk) { }
  ~Status() { }

  Status(ErrorCode code) : code_(code) { }
  Status(ErrorCode code, const std::string& msg) : Status(code) { }

  Status(const Status& s) : code_(s.code_) { }
  void operator=(const Status& s) {
    this->code_ = s.code_;
  }

  Status(Status&& s) : code_(s.code_) { }
  void operator=(Status&& s) { this->code_ = s.code_; }

  bool IsOk() const { return code_ == kOk; }

  ErrorCode Code() const {
    return code_;
  }

  std::string Msg() const {
    return ToString();
  }

  bool operator==(const Status& x) const {
    return x.code_ == this->code_;
  }
  bool operator!=(const Status& x) const {
    return !(*this == x);
  }

  std::string ToString() const {
    std::string error_msg;
    switch (code_) {
      case kOk:
        return "OK";

      case kOutOfRange:
        error_msg = "Out of range";
        break;
      case kOverflow:
        error_msg = "Array Overflow";
        break;
      case kFileSeekError:
        error_msg = "Seek file error";
        break;
      case kFileWriteError:
        error_msg = "Write file error";
        break;
      case kBadArg:
        error_msg = "Bad argument";
        break;
      case kBadData:
        error_msg = "Bad data";
        break;
      case kNoData:
        error_msg = "No data available";
        break;
      case kUnavailable:
        error_msg = "Unavailable";
        break;
      case kNotFound:
        error_msg = "Not found";
        break;
      case kNoMemory:
        error_msg = "No more memory";
        break;
      case kEAccess:
        error_msg = "Access error";
        break;
      case kInternal:
        error_msg = "Internal error";
        break;

      case kUnknown:
      default:
        error_msg = "Unkown error";
        break;
    }
    return error_msg + " " + *ErrorDetailString::Get();  
  }

  static Status Ok() { return Status(); }

#define ADD_UTILITY(name, code)                   \
  static Status name(const std::string& msg) {    \
    ErrorDetailString::Get()->assign(msg);        \
    return Status(code, msg);                     \
  }                                               \
  static Status name() {                          \
    ErrorDetailString::Get()->clear();            \
    return Status(code);                          \
  }

  ADD_UTILITY(OutOfRange,      kOutOfRange       )
  ADD_UTILITY(Overflow,        kOverflow         )
  ADD_UTILITY(FileSeekError,   kFileSeekError    )
  ADD_UTILITY(FileWriteError,  kFileWriteError   )
  ADD_UTILITY(BadArg,          kBadArg           )
  ADD_UTILITY(BadData,         kBadData          )
  ADD_UTILITY(NoData,          kNoData           )
  ADD_UTILITY(Unavailable,     kUnavailable      )
  ADD_UTILITY(NotFound,        kNotFound         )
  ADD_UTILITY(NoMemory,        kNoMemory         )
  ADD_UTILITY(EAccess,         kEAccess          )
  ADD_UTILITY(Internal,        kInternal         )

  ADD_UTILITY(Unknown,         kUnknown          )

#undef ADD_UTILITY

 private:
  ErrorCode code_;
};

}  // namespace common
}  // namespace faststdb

#define CHECK_STATUS(STATUS)			                 \
  do {						                                 \
    faststdb::common::Status __st__ = STATUS;			 \
    if (!__st__.IsOk()) {			                     \
      return __st__;				                       \
    }						                                   \
  } while (0)

#endif  // FASTSTDB_COMMON_STATUS_H_
