/**
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file operator.h contains operator interfaces.
 * Operator performs some processing on data. Operator can work on series level. In this case
 * it doesn't know anything about other series (or columns). Good example of such operator is
 * an aggregate operator that computes some aggregate function on data. Operator can work on
 * tuple level. Tuples are produced from individual series through materialization procedure.
 * Example of such operator is a join operator. This operator consumes several series operators
 * and produces sequence of tuples.
 */
#ifndef FASTSTDB_STORAGE_OPERATORS_OPERATOR_H_
#define FASTSTDB_STORAGE_OPERATORS_OPERATOR_H_

#include "faststdb/common/basic.h"
#include "faststdb/storage/nbtree_def.h"

namespace faststdb {
namespace storage {

enum class AggregationFunction {
  MIN,
  MAX,
  SUM,
  CNT,
  MIN_TIMESTAMP,
  MAX_TIMESTAMP,
  MEAN,
  LAST,
  FIRST,
  LAST_TIMESTAMP,
  FIRST_TIMESTAMP,
};

//! Result of the aggregation operation that has several components.
struct AggregationResult {
  double cnt;
  double sum;
  double min;
  double max;
  double first;
  double last;
  Timestamp mints;
  Timestamp maxts;
  Timestamp _begin;
  Timestamp _end;

  //! Copy all components from subtree reference.
  void copy_from(SubtreeRef const&);
  //! Calculate values from raw data.
  void do_the_math(Timestamp *tss, double const* xss, size_t size, bool inverted);
  /**
   * Add value to aggregate
   * @param ts is a timestamp
   * @param xs is a value
   * @param forward is used to indicate external order of added elements
   */
  void add(Timestamp ts, double xs, bool forward);
  //! Combine this value with the other one (inplace update).
  void combine(const AggregationResult& other);
};

static const AggregationResult INIT_AGGRES = {
  .0,
  .0,
  std::numeric_limits<double>::max(),
  std::numeric_limits<double>::lowest(),
  .0,
  .0,
  std::numeric_limits<Timestamp>::max(),
  std::numeric_limits<Timestamp>::lowest(),
  std::numeric_limits<Timestamp>::max(),
  std::numeric_limits<Timestamp>::lowest(),
};

/** Single series operator.
 * @note all ranges is semi-open. This means that if we're
 *       reading data from A to B, operator should return
 *       data in range [A, B), and B timestamp should be
 *       greater (or less if we're reading data in backward
 *       direction) then all timestamps that we've read before.
 */
template <class TValue>
struct SeriesOperator {
  //! Iteration direction
  enum class Direction {
    FORWARD, BACKWARD,
  };

  //! D-tor
  virtual ~SeriesOperator() = default;

  /** Read next portion of data.
   * @param destts Timestamps destination buffer. On success timestamps will be written here.
   * @param destval Values destination buffer.
   * @param size Size of the  destts and destval buffers (should be the same).
   * @return status and number of elements written to both buffers.
   */
  virtual std::tuple<common::Status, size_t> read(Timestamp* destts, TValue* destval, size_t size) = 0;

  virtual Direction get_direction() = 0;
};

//! Base class for all raw data iterators.
using RealValuedOperator = SeriesOperator<double>;

//! Base class for all aggregating iterators. Return single value.
using AggregateOperator = SeriesOperator<AggregationResult>;

//! Base class for all event iterators.
using BinaryDataOperator = SeriesOperator<std::string>;

/** This interface is used by column-store internally.
 * It materializes tuples/values and produces a series of Sample values.
 */
struct ColumnMaterializer {

  virtual ~ColumnMaterializer() = default;

  /** Read samples in batch.
   * Samples can be of variable size.
   * @param dest is a pointer to buffer that will receive series of Sample values
   * @param size is a size of the buffer in bytes
   * @return status of the operation (success or error code) and number of written bytes
   */
  virtual std::tuple<common::Status, size_t> read(u8 *dest, size_t size) = 0;
};

enum class RangeOverlap {
  NO_OVERLAP,
  FULL_OVERLAP,
  PARTIAL_OVERLAP
};

struct ValueFilter {
  enum {
    LT = 0,  //! Less than
    LE = 1,  //! Less or equal
    GT = 2,  //! Greater than
    GE = 3,  //! Greater or equal
    MAX_INDEX = 4,
  };

  //! Encode threshold mask
  int    mask;
  //! Thresholds
  double thresholds[4];

  ValueFilter();

  bool match(double value) const;

  /**
   * @brief Get rank
   * Filter rank is 0 if it's empty.
   * Filter rank is 1 if only one bound is set (lower or upper).
   * Filter rank is 2 if both bounds are set.
   * @return filter rank
   */
  int get_rank() const;

  /**
   * Return true if the filter is ordered (lowerbound is less than upperbound).
   * Onesided filter is always ordered.
   */
  bool is_ordered() const;

  RangeOverlap get_overlap(const SubtreeRef& ref) const;

  ValueFilter& less_than(double value);

  ValueFilter& less_or_equal(double value);

  ValueFilter& greater_than(double value);

  ValueFilter& greater_or_equal(double value);

  //! Check filter invariant
  bool validate() const;
};

struct AggregateFilter {
  enum {
    AVG,
    MIN,
    MAX,
    FIRST,
    LAST,
    N = 5,
  };

  enum class Mode {
    ALL,
    ANY
  };

  ValueFilter filters[N];
  u32 bitmap;
  Mode mode;

  AggregateFilter();

  /** Set filter, return true on success.
   * @param op is an aggregate name (AVG, MIN...)
   * @param filter is a filter that shuld be used for such aggregate
   * @return true if set, false otherwise
   */
  bool set_filter(u32 op, const ValueFilter& filter);

  bool match(const AggregationResult& res) const;
};

}  // namespace storage
}  // namespace faststdb

#endif  // FASTSTDB_STORAGE_OPERATORS_OPERATOR_H_
