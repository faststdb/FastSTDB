/*!
 * \file basic.h
 */
#ifndef FASTSTDB_COMMON_BASIC_H_
#define FASTSTDB_COMMON_BASIC_H_

#include <stdint.h>

#include <string>

namespace faststdb {

// Some type definitions.
typedef uint64_t u64;
typedef int64_t  i64;
typedef uint32_t u32;
typedef int32_t  i32;
typedef uint16_t u16;
typedef int16_t  i16;
typedef uint8_t  u8;
typedef int8_t   i8;

#define UNUSED(x) (void)(x)
#define FASTSTDB_VERSION 101

typedef u64 Timestamp;  //< Timestamp
typedef u64 ParamId;    //< Parameter (or sequence) id

#define FASTSTDB_MIN_TIMESTAMP 0ull

//! Payload data
typedef struct {
  //------------------------------------------//
  //       Normal payload (float value)       //
  //------------------------------------------//

  //! Value
  double float64;

  /** Payload size (payload can be variably sized)
   *  size = 0 means size = sizeof(Sample)
   */
  u16 size;

  //! Data element flags
  enum {
    REGULLAR         = 1 << 8,  /** indicates that the sample is a part of regullar time-series */
    PARAMID_BIT      = 1,       /** indicates that the param id is set */
    TIMESTAMP_BIT    = 1 << 1   /** indicates that the timestamp is set */,
    CUSTOM_TIMESTAMP = 1 << 2,  /** indicates that timestamp shouldn't be formatted during output */
    FLOAT_BIT        = 1 << 4,  /** scalar type */
    TUPLE_BIT        = 1 << 5,  /** tuple type */
    EVENT_BIT        = 1 << 6,  /** event type */
    SAX_WORD         = 1 << 10, /** indicates that SAX word is stored in extra payload */
  };
  u16 type;

  //---------------------------//
  //       Extra payload       //
  //---------------------------//

  //! Extra payload data
  char data[0];
} PData;

#define PAYLOAD_FLOAT (PData::PARAMID_BIT | PData::TIMESTAMP_BIT | PData::FLOAT_BIT)
#define PAYLOAD_TUPLE (PData::PARAMID_BIT | PData::TIMESTAMP_BIT | PData::TUPLE_BIT)
#define PAYLOAD_EVENT (PData::PARAMID_BIT | PData::TIMESTAMP_BIT | PData::EVENT_BIT)
#define PAYLOAD_NONE  (PData::PARAMID_BIT | PData::TIMESTAMP_BIT)

//! Cursor result type
typedef struct {
  Timestamp timestamp;
  ParamId   paramid;
  PData     payload;
} Sample;

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
