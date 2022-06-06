/*!
 * \file join.h
 */
#ifndef FASTSTDB_STORAGE_OPERATORS_JOIN_H_
#define FASTSTDB_STORAGE_OPERATORS_JOIN_H_

#include "faststdb/storage/operators/join.h"
#include "faststdb/storage/operators/merge.h"

namespace faststdb {
namespace storage {

/** Operator that can be used to join several series.
 * This materializer is based on merge-join but returns tuples ordered by time
 * instead of individual values.
 * Tuple can contain up to 58 elements.
 */
class JoinMaterializer : public ColumnMaterializer {

  std::unique_ptr<ColumnMaterializer> merge_;         //< underlying merge-iterator
  std::vector<ParamId>                orig_ids_;      //< array of original ids
  ParamId                             id_;            //< id of the resulting time-series
  Timestamp                           curr_;          //< timestamp of the currently processed sample
  std::vector<u8>                     buffer_;        //< the read buffer
  u32                                 buffer_size_;   //< read buffer size (capacity is defined by the vector size)
  u32                                 buffer_pos_;    //< position in the read buffer
  const u32                           max_ssize_;     //< element size (in bytes)

 public:
  /**
   * @brief JoinMaterializer2 c-tor
   * @param ids is a original ids of the series
   * @param iters is an array of scan operators
   * @param id is an id of the resulting series
   */
  JoinMaterializer(std::vector<ParamId> &&ids,
                   std::vector<std::unique_ptr<RealValuedOperator>>&& iters,
                   ParamId id);

  /**
   * @brief Read materialized value into buffer
   * Read values to buffer. Values is Sample with variable sized payload.
   * Format: float64 contains bitmap, data contains array of nonempty values (whether a
   * value is empty or not is defined by bitmap)
   * @param dest is a pointer to recieving buffer
   * @param size is a size of the recieving buffer
   * @return status and output size (in bytes)
   */
  std::tuple<common::Status, size_t> read(u8 *dest, size_t size);

 private:
  common::Status fill_buffer();
};

struct JoinConcatMaterializer : ColumnMaterializer {
  std::vector<std::unique_ptr<ColumnMaterializer>> iters_;
  size_t ix_;

  JoinConcatMaterializer(std::vector<std::unique_ptr<ColumnMaterializer>>&& iters)
      : iters_(std::move(iters)), ix_(0) { }

  /** Read values to buffer. Values is Sample with variable sized payload.
   * Format: float64 contains bitmap, data contains array of nonempty values (whether a
   * value is empty or not is defined by bitmap)
   * @param dest is a pointer to recieving buffer
   * @param size is a size of the recieving buffer
   * @return status and output size (in bytes)
   */
  std::tuple<common::Status, size_t> read(u8 *dest, size_t size) {
    while(true) {
      if (ix_ >= iters_.size()) {
        return std::make_tuple(common::Status::NoData(""), 0);
      }
      common::Status status;
      size_t outsz;
      std::tie(status, outsz) = iters_.at(ix_)->read(dest, size);
      if (status.Code() == common::Status::kNoData) {
        ix_++;
        if (outsz != 0) {
          return std::make_tuple(ix_ != iters_.size() ? common::Status::Ok() : common::Status::NoData(""), outsz);
        }
      } else {
        return std::make_tuple(status, outsz);
      }
    }
  }
};

}  // namespace storage
}  // namespace faststdb

#endif  // FASTSTDB_STORAGE_OPERATORS_JOIN_H_
