/*!
 * \file rwlock.h
 */
#ifndef FASTSTDB_COMMON_RWLOCK_H_
#define FASTSTDB_COMMON_RWLOCK_H_

#include "pthread.h"

namespace faststdb {
namespace common {

/**
 * Reader writer lock mutex.
 */
class RWLock {
  pthread_rwlock_t rwlock_;

 public:
  RWLock();

  RWLock(RWLock const&) = delete;
  RWLock(RWLock &&) = delete;
  RWLock& operator = (RWLock const&) = delete;

  ~RWLock();

  void rdlock();

  bool try_rdlock();

  void wrlock();

  bool try_wrlock();

  void unlock();
};

template<class T, void (T::*on_enter)()>
struct LockGuard {
  T& lock;

  LockGuard(T& lock) : lock(lock) {
    (lock.*on_enter)();
  }

  LockGuard(LockGuard const&) = delete;
  LockGuard(LockGuard &&) = delete;
  LockGuard& operator = (LockGuard const&) = delete;

  ~LockGuard() {
    lock.unlock();
  }
};

using UniqueLock = LockGuard<RWLock, &RWLock::wrlock>;
using SharedLock = LockGuard<RWLock, &RWLock::wrlock>;

}  // namespace common
}  // namespace faststdb

#endif  // FASTSTDB_COMMON_RWLOCK_H_
