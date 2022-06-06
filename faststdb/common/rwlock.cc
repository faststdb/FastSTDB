/*!
 * \file rwlock.cc
 */
#include "faststdb/common/rwlock.h"

#include "faststdb/common/logging.h"

namespace faststdb {
namespace common {

RWLock::RWLock() : rwlock_ PTHREAD_RWLOCK_INITIALIZER {
  int error = pthread_rwlock_init(&rwlock_, nullptr);
  if (error) {
    LOG(FATAL) << "pthread_rwlock_init error";
  }
}

RWLock::~RWLock() {
  pthread_rwlock_destroy(&rwlock_);
}

void RWLock::rdlock() {
  int err = pthread_rwlock_rdlock(&rwlock_);
  if (err) {
    LOG(FATAL) << "pthread_rwlock_rdlock error";
  }
}

bool RWLock::try_rdlock() {
  int err = pthread_rwlock_tryrdlock(&rwlock_);
  switch(err) {
    case 0:
      return true;
    case EBUSY:
    case EDEADLK:
      return false;
    default:
      break;
  }
  LOG(FATAL) << "pthread_rwlock_tryrdlock error";
}

void RWLock::wrlock() {
  int err = pthread_rwlock_wrlock(&rwlock_);
  if (err) {
    LOG(FATAL) << "pthread_rwlock_wrlock error";
  }
}

bool RWLock::try_wrlock() {
  int err = pthread_rwlock_trywrlock(&rwlock_);
  switch(err) {
    case 0:
      return true;
    case EBUSY:
    case EDEADLK:
      return false;
    default:
      break;
  }
  LOG(FATAL) << "pthread_rwlock_trywrlock error";
}

void RWLock::unlock() {
  int err = pthread_rwlock_unlock(&rwlock_);
  if (err) {
    LOG(FATAL) << "pthread_rwlock_unlock error";
  }
}

}  // namespace common
}  // namespace faststdb
