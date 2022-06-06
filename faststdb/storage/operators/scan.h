/*!
 * \file scan.h
 */
#ifndef FASTSTDB_STORAGE_OPERATORS_SCAN_H_
#define FASTSTDB_STORAGE_OPERATORS_SCAN_H_

#include "faststdb/storage/operators/operator.h"

namespace faststdb {
namespace storage {

/** Concatenating iterator.
 * Accepts list of iterators in the c-tor. All iterators then
 * can be seen as one iterator. Iterators should be in correct
 * order.
 */
struct ChainOperator : RealValuedOperator {
  typedef std::vector<std::unique_ptr<RealValuedOperator>> IterVec;
  IterVec   iter_;
  Direction dir_;
  u32       iter_index_;

  //! C-tor. Create iterator from list of iterators.
  template<class TVec>
  ChainOperator(TVec&& iter) : iter_(std::forward<TVec>(iter)), iter_index_(0) {
    if (iter_.empty()) {
      dir_ = Direction::FORWARD;
    } else {
      dir_ = iter_.front()->get_direction();
    }
  }

  virtual std::tuple<common::Status, size_t> read(Timestamp *destts, double *destval, size_t size);
  virtual Direction get_direction();
};

/**
 * Materializes list of columns by chaining them
 */
class ChainMaterializer : public ColumnMaterializer {
  std::vector<std::unique_ptr<RealValuedOperator>> iters_;
  std::vector<ParamId> ids_;
  size_t pos_;
 
 public:
  ChainMaterializer(std::vector<ParamId>&& ids, std::vector<std::unique_ptr<RealValuedOperator>>&& it);
  virtual std::tuple<common::Status, size_t> read(u8 *dest, size_t size);
};

/**
 * Materializes list of event columns by chaining them
 */
class EventChainMaterializer : public ColumnMaterializer {
  std::vector<std::unique_ptr<BinaryDataOperator>> iters_;
  std::vector<ParamId> ids_;
  size_t pos_;
  bool available_;
  std::string curr_;
  ParamId curr_id_;
  Timestamp curr_ts_;

 public:
  EventChainMaterializer(std::vector<ParamId>&& ids, std::vector<std::unique_ptr<BinaryDataOperator>>&& it);
  virtual std::tuple<common::Status, size_t> read(u8 *dest, size_t size);
};

}  // namespace storage
}  // namespace faststdb

#endif  // FASTSTDB_STORAGE_OPERATORS_SCAN_H_
