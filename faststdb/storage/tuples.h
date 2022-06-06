/*!
 * \file tuples.h
 */
#ifndef FASTSTDB_STORAGE_TUPLES_H_
#define FASTSTDB_STORAGE_TUPLES_H_

#include <tuple>
#include <vector>
#include <cassert>

#include "faststdb/common/basic.h"
#include "faststdb/storage/operators/operator.h"

namespace faststdb {
namespace storage {

struct TupleOutputUtils {

  /** Get pointer to buffer and return pointer to sample and tuple data */
  static std::tuple<Sample*, double*> cast(u8* dest) {
    Sample* sample = reinterpret_cast<Sample*>(dest);
    double* tuple  = reinterpret_cast<double*>(sample->payload.data);
    return std::make_tuple(sample, tuple);
  }

  static double get_flags(std::vector<AggregationFunction> const& tup) {
    // Shift will produce power of two (e.g. if tup.size() == 3 then
    // (1 << tup.size) will give us 8, 8-1 is 7 (exactly three lower
    // bits is set)).
    union {
      double d;
      u64 u;
    } bits;
    bits.u = (1ull << tup.size()) - 1;
    // Save number of elements in the bitflags
    bits.u |= (u64)tup.size() << 58;
    return bits.d;
  }

  // Returns size of the tuple and bitmap
  static std::tuple<u32, u64> get_size_and_bitmap(double value) {
    union {
      double d;
      u64 u;
    } bits;
    bits.d = value;
    u32 size = static_cast<u32>(bits.u >> 58);
    u32 bitmap = 0x3ffffffffffffff & bits.u;
    return std::make_tuple(size, bitmap);
  }

  static double get(AggregationResult const& res, AggregationFunction afunc) {
    double out = 0;
    switch (afunc) {
      case AggregationFunction::CNT:
        out = res.cnt;
        break;
      case AggregationFunction::SUM:
        out = res.sum;
        break;
      case AggregationFunction::MIN:
        out = res.min;
        break;
      case AggregationFunction::MIN_TIMESTAMP:
        out = static_cast<double>(res.mints);
        break;
      case AggregationFunction::MAX:
        out = res.max;
        break;
      case AggregationFunction::MAX_TIMESTAMP:
        out = res.maxts;
        break;
      case AggregationFunction::MEAN:
        out = res.sum / res.cnt;
        break;
      case AggregationFunction::LAST:
        out = res.last;
        break;
      case AggregationFunction::FIRST:
        out = res.first;
        break;
      case AggregationFunction::LAST_TIMESTAMP:
        out = res._end;
        break;
      case AggregationFunction::FIRST_TIMESTAMP:
        out = res._begin;
        break;
    }
    return out;
  }

  static void set_tuple(double* tuple, std::vector<AggregationFunction> const& comp, AggregationResult const& res) {
    for (size_t i = 0; i < comp.size(); i++) {
      auto elem = comp[i];
      *tuple = get(res, elem);
      tuple++;
    }
  }

  static size_t get_tuple_size(const std::vector<AggregationFunction>& tup) {
    size_t payload = 0;
    assert(!tup.empty());
    payload = sizeof(double) * tup.size();
    return sizeof(Sample) + payload;
  }

  static double get_first_value(const Sample* sample) {
    union {
      double d;
      u64 u;
    } bits;
    bits.d = sample->payload.float64;
    UNUSED(bits);
    assert(sample->payload.type == PAYLOAD_TUPLE && bits.u == 0x400000000000001ul);
    const double* value = reinterpret_cast<const double*>(sample->payload.data);
    return *value;
  }

  static void set_first_value(Sample* sample, double x) {
    union {
      double d;
      u64 u;
    } bits;
    bits.d = sample->payload.float64;
    UNUSED(bits);
    assert(sample->payload.type == PAYLOAD_TUPLE && bits.u == 0x400000000000001ul);
    double* value = reinterpret_cast<double*>(sample->payload.data);
    *value = x;
  }

  static Sample* copy_sample(const Sample* src, char* dest, size_t dest_size) {
    size_t sample_size = std::max(static_cast<size_t>(src->payload.size), sizeof(Sample));
    if (sample_size > dest_size) {
      return nullptr;
    }
    memcpy(dest, src, sample_size);
    return reinterpret_cast<Sample*>(dest);
  }

  static bool is_one_element_tuple(const Sample* sample) {
    if (sample->payload.type == PAYLOAD_TUPLE) {
      union {
        double d;
        u64 u;
      } bits;
      bits.d = sample->payload.float64;
      if (bits.u != 0x400000000000001ul) {
        // only one element tuples supported
        return false;
      }
      return true;
    }
    return false;
  }
};

}  // namespace storage
}  // namespace faststdb

#endif  // FASTSTDB_STORAGE_TUPLES_H_
