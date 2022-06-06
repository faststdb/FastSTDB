/*!
 * \file crc32.h
 */
#ifndef FASTSTDB_COMMON_CRC32_H_
#define FASTSTDB_COMMON_CRC32_H_

#include <stdint.h>
#include <stddef.h>

namespace faststdb {
namespace common {

typedef uint32_t (*crc32c_impl_t)(uint32_t crc, const void *buf, size_t len);

enum class CRC32C_hint {
  DETECT,
  FORCE_SW,
  FORCE_HW,
};

//! Return crc32c implementation.
crc32c_impl_t chose_crc32c_implementation(CRC32C_hint hint=CRC32C_hint::DETECT);

}  // namespace common
}  // namespace faststdb

#endif  // FASTSTDB_COMMON_CRC32_H_
