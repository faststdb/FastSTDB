/*!
 * \file basic.h
 */
#ifndef FASTSTDB_COMMON_BASIC_H_
#define FASTSTDB_COMMON_BASIC_H_

#include <stdint.h>

#include <string>

#include "faststdb/common/types.h"
#include "faststdb/common/config.h"

namespace faststdb {

// Some type definitions.
#ifdef __GNUC__
#define LIKELY(x)   __builtin_expect((x), 1)
#define UNLIKELY(x) __builtin_expect((x), 0)
#else
#define LIKELY(x)   (x)
#define UNLIKELY(x) (x)
#endif

#define PACKED __attribute__((__packed__))
#define UNUSED(x) (void)(x)
#define LIMITS_MAX_SNAME 0x1000
#define LIMITS_MAX_TAGS 32
#define MAX_THREADS 1024

#define FASTSTDB_VERSION 101

#define FASTSTDB_MIN_TIMESTAMP 0ull
#define FASTSTDB_MAX_TIMESTAMP (~0ull)
#define FASTSTDB_LIMITS_MAX_EVENT_LEN 4096

inline bool same_value(double a, double b) {
  union Bits {
    double d;
    u64 u;
  };
  Bits ba = {};
  ba.d = a;
  Bits bb = {};
  bb.d = b;
  return ba.u == bb.u;
}

namespace common {

inline std::string GetMetaVolumeDir() {
  auto ptr = getenv("HOME");
  if (ptr) {
    return std::string(ptr) + "/.faststdb";
  } else {
    return ".faststdb";
  }
}

}  // namespace common
}  // namespace faststdb

#endif  // FASTSTDB_COMMON_BASIC_H_
