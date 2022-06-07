/*!
 * \file status.h
 */
#ifndef FASTSTDB_COMMON_STATUS_H_
#define FASTSTDB_COMMON_STATUS_H_

#include <memory>
#include <string>
#include <algorithm>

namespace faststdb {
namespace common {

class Status {
 public:
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

  Status() : state_(nullptr) {}
  ~Status() { delete state_; }

  Status(ErrorCode code, const std::string& msg)
    : state_(new State{code, msg}) {}

  Status(const Status& s) : state_(s.state_ == nullptr ? nullptr : new State(*s.state_)) {}
  void operator=(const Status& s) {
    if (!state_) delete state_;
    state_ = s.state_ == nullptr ? nullptr : new State(*s.state_);
  }

  Status(Status&& s) : state_(s.state_) { s.state_ = nullptr; }
  void operator=(Status&& s) { std::swap(state_, s.state_); }

  bool IsOk() const { return state_ == nullptr; }

  ErrorCode Code() const {
    return state_ == nullptr ? kOk : state_->code;
  }

  std::string Msg() const {
    return state_ == nullptr ? "" : state_->msg;
  }

  bool operator==(const Status& x) const {
    return (state_ == nullptr && x.state_ == nullptr) ||
      (state_ != nullptr && x.state_ != nullptr && state_->code == x.state_->code && state_->msg == x.state_->msg);
  }
  bool operator!=(const Status& x) const {
    return !(*this == x);
  }

  std::string ToString() const {
    if (state_ == nullptr) {
      return "OK";
    } else {
      return "ErrorCode [" + std::to_string(Code()) + "]: " + Msg();
    }
  }

  static Status Ok() { return Status(); }
  static Status OutOfRange(const std::string& msg) { return Status(kOutOfRange, msg); }
  static Status Overflow(const std::string& msg) { return Status(kOverflow, msg); }
  static Status FileSeekError(const std::string& msg) { return Status(kFileSeekError, msg); }
  static Status FileWriteError(const std::string& msg) { return Status(kFileWriteError, msg); }
  static Status BadArg(const std::string& msg) { return Status(kBadArg, msg); }
  static Status BadData(const std::string& msg) { return Status(kBadData, msg); }
  static Status NoData(const std::string& msg) { return Status(kNoData, msg); }
  static Status Unavailable(const std::string& msg) { return Status(kUnavailable, msg); }
  static Status NotFound(const std::string& msg) { return Status(kNotFound, msg); }
  static Status NoMemory(const std::string& msg) { return Status(kNoMemory, msg); }
  static Status EAccess(const std::string& msg) { return Status(kEAccess, msg); }
  static Status Internal(const std::string& msg) { return Status(kInternal, msg); }
  static Status Unknown(const std::string& msg) { return Status(kUnknown, msg); }

 private:
  struct State {
    ErrorCode code;
    std::string msg;
  };

  State* state_;
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
