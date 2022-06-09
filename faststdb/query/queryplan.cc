/*!
 * \file queryplan.cc
 */
#include "faststdb/query/queryplan.h"

#include "faststdb/storage/nbtree.h"
#include "faststdb/storage/column_store.h"
#include "faststdb/storage/operators/operator.h"
#include "faststdb/storage/operators/scan.h"
#include "faststdb/storage/operators/merge.h"
#include "faststdb/storage/operators/aggregate.h"
#include "faststdb/storage/operators/join.h"

namespace faststdb {
namespace qp {

using namespace storage;

/**
 * Tier-1 operator
 */
struct ProcessingPrelude {
  virtual ~ProcessingPrelude() = default;
  //! Compute processing step result (list of low level operators)
  virtual common::Status apply(const ColumnStore& cstore) = 0;
  //! Get result of the processing step
  virtual common::Status extract_result(std::vector<std::unique_ptr<RealValuedOperator>>* dest) = 0;
  //! Get result of the processing step
  virtual common::Status extract_result(std::vector<std::unique_ptr<AggregateOperator>>* dest) = 0;
  //! Get result of the processing step
  virtual common::Status extract_result(std::vector<std::unique_ptr<BinaryDataOperator>>* dest) = 0;
};

/**
 * Tier-N operator (materializer)
 */
struct MaterializationStep {
  virtual ~MaterializationStep() = default;

  //! Compute processing step result (list of low level operators)
  virtual common::Status apply(ProcessingPrelude* prelude) = 0;

  /**
   * Get result of the processing step, this method should add cardinality() elements
   * to the `dest` array.
   */
  virtual common::Status extract_result(std::unique_ptr<ColumnMaterializer>* dest) = 0;
};

struct ScanProcessingStep : ProcessingPrelude {
  std::vector<std::unique_ptr<RealValuedOperator>> scanlist_;
  Timestamp begin_;
  Timestamp end_;
  std::vector<ParamId> ids_;

  template<class T>
  ScanProcessingStep(Timestamp begin, Timestamp end, T&& t)
    : begin_(begin)
      , end_(end)
      , ids_(std::forward<T>(t))
  {
  }

  virtual common::Status apply(const ColumnStore& cstore) {
    return cstore.scan(ids_, begin_, end_, &scanlist_);
  }

  virtual common::Status extract_result(std::vector<std::unique_ptr<RealValuedOperator>>* dest) {
    if (scanlist_.empty()) {
      return common::Status::NoData();
    }
    *dest = std::move(scanlist_);
    return common::Status::Ok();
  }

  virtual common::Status extract_result(std::vector<std::unique_ptr<AggregateOperator>>* dest) {
    return common::Status::NoData();
  }

  virtual common::Status extract_result(std::vector<std::unique_ptr<BinaryDataOperator>>* dest) {
    return common::Status::NoData();
  }
};

struct ScanEventsProcessingStep : ProcessingPrelude {
  std::vector<std::unique_ptr<BinaryDataOperator>> scanlist_;
  Timestamp begin_;
  Timestamp end_;
  std::vector<ParamId> ids_;
  std::string regex_;

  //! C-tor (1), create scan without filter
  template<class T>
  ScanEventsProcessingStep(Timestamp begin, Timestamp end, T&& t)
        : begin_(begin)
        , end_(end)
        , ids_(std::forward<T>(t))
  {
  }

  //! C-tor (2), create scan with filter
  template<class T>
  ScanEventsProcessingStep(Timestamp begin, Timestamp end, const std::string& exp, T&& t)
        : begin_(begin)
        , end_(end)
        , ids_(std::forward<T>(t))
        , regex_(exp)
  {
  }

  virtual common::Status apply(const ColumnStore& cstore) {
    if (!regex_.empty()) {
      return cstore.filter_events(ids_, begin_, end_, regex_, &scanlist_);
    }
    return cstore.scan_events(ids_, begin_, end_, &scanlist_);
  }

  virtual common::Status extract_result(std::vector<std::unique_ptr<BinaryDataOperator>>* dest) {
    if (scanlist_.empty()) {
      return common::Status::NoData();
    }
    *dest = std::move(scanlist_);
    return common::Status::Ok();
  }

  virtual common::Status extract_result(std::vector<std::unique_ptr<RealValuedOperator>>* dest) {
    return common::Status::NoData();
  }

  virtual common::Status extract_result(std::vector<std::unique_ptr<AggregateOperator>>* dest) {
    return common::Status::NoData();
  }
};

struct FilterProcessingStep : ProcessingPrelude {
  std::vector<std::unique_ptr<RealValuedOperator>> scanlist_;
  Timestamp begin_;
  Timestamp end_;
  std::map<ParamId, ValueFilter> filters_;
  std::vector<ParamId> ids_;

  template<class T>
  FilterProcessingStep(Timestamp begin,
                       Timestamp end,
                       const std::vector<ValueFilter>& flt,
                       T&& t)
        : begin_(begin)
        , end_(end)
        , filters_()
        , ids_(std::forward<T>(t))
  {
    for (size_t ix = 0; ix < ids_.size(); ix++) {
      ParamId id = ids_[ix];
      const ValueFilter& filter = flt[ix];
      filters_.insert(std::make_pair(id, filter));
    }
  }

  virtual common::Status apply(const ColumnStore& cstore) {
    return cstore.filter(ids_, begin_, end_, filters_, &scanlist_);
  }

  virtual common::Status extract_result(std::vector<std::unique_ptr<RealValuedOperator>>* dest) {
    if (scanlist_.empty()) {
      return common::Status::NoData();
    }
    *dest = std::move(scanlist_);
    return common::Status::Ok();
  }

  virtual common::Status extract_result(std::vector<std::unique_ptr<AggregateOperator>>* dest) {
    return common::Status::NoData();
  }

  virtual common::Status extract_result(std::vector<std::unique_ptr<BinaryDataOperator>>* dest) {
    return common::Status::NoData();
  }
};

struct AggregateProcessingStep : ProcessingPrelude {
  std::vector<std::unique_ptr<AggregateOperator>> agglist_;
  Timestamp begin_;
  Timestamp end_;
  std::vector<ParamId> ids_;

  template<class T>
  AggregateProcessingStep(Timestamp begin, Timestamp end, T&& t)
        : begin_(begin)
        , end_(end)
        , ids_(std::forward<T>(t))
  {
  }

  virtual common::Status apply(const ColumnStore& cstore) {
    return cstore.aggregate(ids_, begin_, end_, &agglist_);
  }

  virtual common::Status extract_result(std::vector<std::unique_ptr<RealValuedOperator>>* dest) {
    return common::Status::NoData();
  }

  virtual common::Status extract_result(std::vector<std::unique_ptr<AggregateOperator>>* dest) {
    if (agglist_.empty()) {
      return common::Status::NoData();
    }
    *dest = std::move(agglist_);
    return common::Status::Ok();
  }

  virtual common::Status extract_result(std::vector<std::unique_ptr<BinaryDataOperator>>* dest) {
    return common::Status::NoData();
  }
};


// Convert AggregateOperator into RealValuedOperator
class GroupAggregateConverter : public RealValuedOperator {
  std::unique_ptr<AggregateOperator> op_;
  AggregationFunction func_;
 
 public:

  GroupAggregateConverter(AggregationFunction func, std::unique_ptr<AggregateOperator> op)
      : op_(std::move(op))
        , func_(func)
  {
  }

  std::tuple<common::Status, size_t> read(Timestamp *destts, double *destval, size_t size) override {
    size_t pos = 0;
    while (pos < size) {
      common::Status status;
      size_t ressz;
      Timestamp ts;
      AggregationResult xs;
      std::tie(status, ressz) = op_->read(&ts, &xs, 1);
      if (ressz == 1) {
        destts[pos] = ts;
        destval[pos] = TupleOutputUtils::get(xs, func_);
        pos++;
      } else if (status.IsOk() || status.Code() == common::Status::kNoData) {
        return std::make_tuple(status, pos);
      } else {
        return std::make_tuple(status, 0);
      }
    }
    return std::make_tuple(common::Status::Ok(), pos);
  }

  Direction get_direction() override {
    return AggregateOperator::Direction::FORWARD == op_->get_direction()
        ? RealValuedOperator::Direction::FORWARD
        : RealValuedOperator::Direction::BACKWARD;
  }
};


struct GroupAggregateProcessingStep : ProcessingPrelude {
  std::vector<std::unique_ptr<AggregateOperator>> agglist_;
  Timestamp begin_;
  Timestamp end_;
  Timestamp step_;
  std::vector<ParamId> ids_;
  AggregationFunction fn_;

  template<class T>
  GroupAggregateProcessingStep(Timestamp begin, Timestamp end, Timestamp step, T&& t, AggregationFunction fn=AggregationFunction::FIRST)
        : begin_(begin)
        , end_(end)
        , step_(step)
        , ids_(std::forward<T>(t))
        , fn_(fn)
  {
  }

  virtual common::Status apply(const ColumnStore& cstore) {
    return cstore.group_aggregate(ids_, begin_, end_, step_, &agglist_);
  }

  virtual common::Status extract_result(std::vector<std::unique_ptr<RealValuedOperator>>* dest) {
    if (agglist_.empty()) {
      return common::Status::NoData();
    }
    dest->clear();
    for (auto&& it: agglist_) {
      std::unique_ptr<RealValuedOperator> op;
      op.reset(new GroupAggregateConverter(fn_, std::move(it)));
      dest->push_back(std::move(op));
    }
    agglist_.clear();
    return common::Status::Ok();
  }

  virtual common::Status extract_result(std::vector<std::unique_ptr<AggregateOperator>>* dest) {
    if (agglist_.empty()) {
      return common::Status::NoData();
    }
    *dest = std::move(agglist_);
    return common::Status::Ok();
  }

  virtual common::Status extract_result(std::vector<std::unique_ptr<BinaryDataOperator>>* dest) {
    return common::Status::NoData();
  }
};

struct GroupAggregateFilterProcessingStep : ProcessingPrelude {
  std::vector<std::unique_ptr<AggregateOperator>> agglist_;
  Timestamp begin_;
  Timestamp end_;
  Timestamp step_;
  std::vector<ParamId> ids_;
  std::map<ParamId, AggregateFilter> filters_;
  AggregationFunction fn_;

  template<class T>
  GroupAggregateFilterProcessingStep(Timestamp begin,
                                     Timestamp end,
                                     Timestamp step,
                                     const std::vector<AggregateFilter>& flt,
                                     T&& t,
                                     AggregationFunction fn=AggregationFunction::FIRST)
        : begin_(begin)
        , end_(end)
        , step_(step)
        , ids_(std::forward<T>(t))
        , fn_(fn)
  {
    for (size_t ix = 0; ix < ids_.size(); ix++) {
      ParamId id = ids_[ix];
      const auto& filter = flt[ix];
      filters_.insert(std::make_pair(id, filter));
    }
  }

  virtual common::Status apply(const ColumnStore& cstore) {
    return cstore.group_aggfilter(ids_, begin_, end_, step_, filters_, &agglist_);
  }

  virtual common::Status extract_result(std::vector<std::unique_ptr<RealValuedOperator>>* dest) {
    if (agglist_.empty()) {
      return common::Status::NoData();
    }
    dest->clear();
    for (auto&& it: agglist_) {
      std::unique_ptr<RealValuedOperator> op;
      op.reset(new GroupAggregateConverter(fn_, std::move(it)));
      dest->push_back(std::move(op));
    }
    agglist_.clear();
    return common::Status::Ok();
  }

  virtual common::Status extract_result(std::vector<std::unique_ptr<AggregateOperator>>* dest) {
    if (agglist_.empty()) {
      return common::Status::NoData();
    }
    *dest = std::move(agglist_);
    return common::Status::Ok();
  }

  virtual common::Status extract_result(std::vector<std::unique_ptr<BinaryDataOperator>>* dest) {
    return common::Status::NoData();
  }
};

// -------------------------------- //
//              Tier-2              //
// -------------------------------- //
namespace detail {

template <OrderBy order, class OperatorT>
struct MergeMaterializerTraits;

template <>
struct MergeMaterializerTraits<OrderBy::SERIES, RealValuedOperator> {
  typedef MergeMaterializer<SeriesOrder> Materializer;
};

template <>
struct MergeMaterializerTraits<OrderBy::TIME, RealValuedOperator> {
  typedef MergeMaterializer<TimeOrder> Materializer;
};

template <>
struct MergeMaterializerTraits<OrderBy::SERIES, BinaryDataOperator> {
  typedef MergeEventMaterializer<EventSeriesOrder> Materializer;
};

template <>
struct MergeMaterializerTraits<OrderBy::TIME, BinaryDataOperator> {
  typedef MergeEventMaterializer<EventTimeOrder> Materializer;
};

}  // namespace detail

/**
 * Merge several series (order by series).
 * Used in scan query.
 */
template<OrderBy order, class OperatorT=RealValuedOperator>
struct MergeBy : MaterializationStep {
  std::vector<ParamId> ids_;
  std::unique_ptr<ColumnMaterializer> mat_;

  template<class IdVec>
  MergeBy(IdVec&& ids)
      : ids_(std::forward<IdVec>(ids))
  {
  }

  common::Status apply(ProcessingPrelude* prelude) {
    std::vector<std::unique_ptr<OperatorT>> iters;
    auto status = prelude->extract_result(&iters);
    if (!status.IsOk()) {
      return status;
    }
    typedef typename detail::MergeMaterializerTraits<order, OperatorT>::Materializer Merger;
    mat_.reset(new Merger(std::move(ids_), std::move(iters)));
    return common::Status::Ok();
  }

  common::Status extract_result(std::unique_ptr<ColumnMaterializer> *dest) {
    if (!mat_) {
      return common::Status::NoData();
    }
    *dest = std::move(mat_);
    return common::Status::Ok();
  }
};

namespace detail {

template <class OperatorT>
struct ChainMaterializerTraits;

template <>
struct ChainMaterializerTraits<RealValuedOperator> {
  typedef ChainMaterializer Materializer;
};

template <>
struct ChainMaterializerTraits<BinaryDataOperator> {
  typedef EventChainMaterializer Materializer;
};

}  // namespace detail

template <class OperatorT = RealValuedOperator>
struct Chain : MaterializationStep {
  std::vector<ParamId> ids_;
  std::unique_ptr<ColumnMaterializer> mat_;

  template<class IdVec>
  Chain(IdVec&& vec) : ids_(std::forward<IdVec>(vec)) { }

  common::Status apply(ProcessingPrelude *prelude) {
    std::vector<std::unique_ptr<OperatorT>> iters;
    auto status = prelude->extract_result(&iters);
    if (!status.IsOk()) {
      return status;
    }
    typedef typename detail::ChainMaterializerTraits<OperatorT>::Materializer Materializer;
    mat_.reset(new Materializer(std::move(ids_), std::move(iters)));
    return common::Status::Ok();
  }

  common::Status extract_result(std::unique_ptr<ColumnMaterializer> *dest) {
    if (!mat_) {
      return common::Status::NoData();
    }
    *dest = std::move(mat_);
    return common::Status::Ok();
  }
};

/**
 * Aggregate materializer.
 * Accepts the list of ids and the list of aggregate operators.
 * Maps each id to the corresponding operators 1-1.
 * All ids should be different.
 */
struct Aggregate : MaterializationStep {
  std::vector<ParamId> ids_;
  std::vector<AggregationFunction> fn_;
  std::unique_ptr<ColumnMaterializer> mat_;

  template<class IdVec, class FuncVec>
  Aggregate(IdVec&& vec, FuncVec&& fn)
     : ids_(std::forward<IdVec>(vec))
      , fn_(std::forward<FuncVec>(fn))
  {
  }

  common::Status apply(ProcessingPrelude *prelude) {
    std::vector<std::unique_ptr<AggregateOperator>> iters;
    auto status = prelude->extract_result(&iters);
    if (!status.IsOk()) {
      return status;
    }
    mat_.reset(new AggregateMaterializer(std::move(ids_), std::move(iters), std::move(fn_)));
    return common::Status::Ok();
  }

  common::Status extract_result(std::unique_ptr<ColumnMaterializer> *dest) {
    if (!mat_) {
      return common::Status::NoData();
    }
    *dest = std::move(mat_);
    return common::Status::Ok();
  }
};

/**
 * Combines the aggregate operators.
 * Accepts list of ids (shouldn't be different) and list of aggregate
 * operators. Maps each id to operator and then combines operators
 * with the same id (used to implement aggregate + group-by).
 */
struct AggregateCombiner : MaterializationStep {
  std::vector<ParamId> ids_;
  std::vector<AggregationFunction> fn_;
  std::unique_ptr<ColumnMaterializer> mat_;

  template <class IdVec, class FuncVec>
  AggregateCombiner(IdVec&& vec, FuncVec&& fn)
      : ids_(std::forward<IdVec>(vec))
        , fn_(std::forward<FuncVec>(fn))
  {
  }

  common::Status apply(ProcessingPrelude *prelude) {
    std::vector<std::unique_ptr<AggregateOperator>> iters;
    auto status = prelude->extract_result(&iters);
    if (status != common::Status::Ok()) {
      return status;
    }
    std::vector<std::unique_ptr<AggregateOperator>> agglist;
    std::map<ParamId, std::vector<std::unique_ptr<AggregateOperator>>> groupings;
    std::map<ParamId, AggregationFunction> functions;
    for (size_t i = 0; i < ids_.size(); i++) {
      auto id = ids_.at(i);
      auto it = std::move(iters.at(i));
      groupings[id].push_back(std::move(it));
      functions[id] = fn_.at(i);
    }
    std::vector<ParamId> ids;
    std::vector<AggregationFunction> fns;
    for (auto& kv: groupings) {
      auto& vec = kv.second;
      ids.push_back(kv.first);
      std::unique_ptr<CombineAggregateOperator> it(new CombineAggregateOperator(std::move(vec)));
      agglist.push_back(std::move(it));
      fns.push_back(functions[kv.first]);
    }
    mat_.reset(new AggregateMaterializer(std::move(ids),
                                         std::move(agglist),
                                         std::move(fns)));
    return common::Status::Ok();
  }

  common::Status extract_result(std::unique_ptr<ColumnMaterializer> *dest) {
    if (!mat_) {
      return common::Status::NoData();
    }
    *dest = std::move(mat_);
    return common::Status::Ok();
  }
};


template <OrderBy order>
struct GroupAggregateCombiner_Initializer;

template <>
struct GroupAggregateCombiner_Initializer<OrderBy::SERIES> {
  static std::unique_ptr<ColumnMaterializer> make_materializer(
      std::vector<ParamId>&& ids,
      std::vector<std::unique_ptr<AggregateOperator>>&& agglist,
      const std::vector<AggregationFunction>& fn)
  {
    std::unique_ptr<ColumnMaterializer> mat;
    mat.reset(new SeriesOrderAggregateMaterializer(std::move(ids), std::move(agglist), fn));
    return mat;
  }
};

template <>
struct GroupAggregateCombiner_Initializer<OrderBy::TIME> {
  static std::unique_ptr<ColumnMaterializer> make_materializer(
      std::vector<ParamId>&& ids,
      std::vector<std::unique_ptr<AggregateOperator>>&& agglist,
      const std::vector<AggregationFunction>& fn)
  {
    std::vector<ParamId> tmpids(ids);
    std::vector<std::unique_ptr<AggregateOperator>> tmpiters(std::move(agglist));
    std::unique_ptr<ColumnMaterializer> mat;
    mat.reset(new TimeOrderAggregateMaterializer(tmpids, tmpiters, fn));
    return mat;
  }
};

/**
 * Combines the group-aggregate operators.
 * Accepts list of ids (shouldn't be different) and list of aggregate
 * operators. Maps each id to operator and then combines operators
 * with the same id (to implement group-aggregate + group/pivot-by-tag).
 */
template <OrderBy order>
struct GroupAggregateCombiner : MaterializationStep {
  std::vector<ParamId> ids_;
  std::vector<AggregationFunction> fn_;
  std::unique_ptr<ColumnMaterializer> mat_;

  template<class IdVec, class FuncVec>
  GroupAggregateCombiner(IdVec&& vec, FuncVec&& fn)
        : ids_(std::forward<IdVec>(vec))
        , fn_(std::forward<FuncVec>(fn))
  {
  }

  common::Status apply(ProcessingPrelude *prelude) {
    std::vector<std::unique_ptr<AggregateOperator>> iters;
    auto status = prelude->extract_result(&iters);
    if (status != common::Status::Ok()) {
      return status;
    }
    std::vector<std::unique_ptr<AggregateOperator>> agglist;
    std::map<ParamId, std::vector<std::unique_ptr<AggregateOperator>>> groupings;
    for (size_t i = 0; i < ids_.size(); i++) {
      auto id = ids_.at(i);
      auto it = std::move(iters.at(i));
      groupings[id].push_back(std::move(it));
    }
    std::vector<ParamId> ids;
    for (auto& kv: groupings) {
      auto& vec = kv.second;
      ids.push_back(kv.first);
      std::unique_ptr<FanInAggregateOperator> it(new FanInAggregateOperator(std::move(vec)));
      agglist.push_back(std::move(it));
    }
    mat_ = GroupAggregateCombiner_Initializer<order>::make_materializer(std::move(ids),
                                                                        std::move(agglist),
                                                                        fn_);
    return common::Status::Ok();
  }

  common::Status extract_result(std::unique_ptr<ColumnMaterializer> *dest) {
    if (!mat_) {
      return common::Status::NoData();
    }
    *dest = std::move(mat_);
    return common::Status::Ok();
  }
};

/**
 * Joins several operators into one.
 * Number of joined operators is defined by the cardinality.
 * Number of ids should be `cardinality` times smaller than number
 * of operators because every `cardinality` operators are joined into
 * one.
 */
struct Join : MaterializationStep {
  std::vector<ParamId> ids_;
  int cardinality_;
  OrderBy order_;
  Timestamp begin_;
  Timestamp end_;
  std::unique_ptr<ColumnMaterializer> mat_;

  template<class IdVec>
  Join(IdVec&& vec, int cardinality, OrderBy order, Timestamp begin, Timestamp end)
        : ids_(std::forward<IdVec>(vec))
        , cardinality_(cardinality)
        , order_(order)
        , begin_(begin)
        , end_(end)
  {
  }

  common::Status apply(ProcessingPrelude *prelude) {
    int inc = cardinality_;
    std::vector<std::unique_ptr<RealValuedOperator>> scanlist;
    auto status = prelude->extract_result(&scanlist);
    if (status != common::Status::Ok()) {
      return status;
    }
    std::vector<std::unique_ptr<ColumnMaterializer>> iters;
    for (size_t i = 0; i < ids_.size(); i++) {
      // ids_ contain ids of the joined series that corresponds
      // to the names in the series matcher
      std::vector<std::unique_ptr<RealValuedOperator>> joined;
      std::vector<ParamId> ids;
      for (int j = 0; j < inc; j++) {
        // `inc` number of storage level operators correspond to one
        // materializer
        auto ix = i*inc + j;
        joined.push_back(std::move(scanlist.at(ix)));
        ids.push_back(ix);
      }
      std::unique_ptr<ColumnMaterializer> it;
      it.reset(new JoinMaterializer(std::move(ids), std::move(joined), ids_.at(i)));
      iters.push_back(std::move(it));
    }
    if (order_ == OrderBy::SERIES) {
      mat_.reset(new JoinConcatMaterializer(std::move(iters)));
    } else {
      bool forward = begin_ < end_;
      typedef MergeJoinMaterializer<MergeJoinUtil::OrderByTimestamp> Materializer;
      mat_.reset(new Materializer(std::move(iters), forward));
    }
    return common::Status::Ok();
  }

  common::Status extract_result(std::unique_ptr<ColumnMaterializer> *dest) {
    if (!mat_) {
      return common::Status::NoData();
    }
    *dest = std::move(mat_);
    return common::Status::Ok();
  }
};

/**
 * Merges several group-aggregate operators by chaining
 */
struct SeriesOrderAggregate : MaterializationStep {
  std::vector<ParamId> ids_;
  std::vector<AggregationFunction> fn_;
  std::unique_ptr<ColumnMaterializer> mat_;

  template <class IdVec, class FnVec>
  SeriesOrderAggregate(IdVec&& vec, FnVec&& fn)
        : ids_(std::forward<IdVec>(vec))
        , fn_(std::forward<FnVec>(fn))
  {
  }

  common::Status apply(ProcessingPrelude *prelude) {
    std::vector<std::unique_ptr<AggregateOperator>> iters;
    auto status = prelude->extract_result(&iters);
    if (status != common::Status::Ok()) {
      return status;
    }
    mat_.reset(new SeriesOrderAggregateMaterializer(std::move(ids_), std::move(iters), fn_));
    return common::Status::Ok();
  }

  common::Status extract_result(std::unique_ptr<ColumnMaterializer> *dest) {
    if (!mat_) {
      return common::Status::NoData();
    }
    *dest = std::move(mat_);
    return common::Status::Ok();
  }
};

struct TimeOrderAggregate : MaterializationStep {
  std::vector<ParamId> ids_;
  std::vector<AggregationFunction> fn_;
  std::unique_ptr<ColumnMaterializer> mat_;

  template<class IdVec, class FnVec>
  TimeOrderAggregate(IdVec&& vec, FnVec&& fn)
        : ids_(std::forward<IdVec>(vec))
        , fn_(std::forward<FnVec>(fn))
  {
  }

  common::Status apply(ProcessingPrelude *prelude) {
    std::vector<std::unique_ptr<AggregateOperator>> iters;
    auto status = prelude->extract_result(&iters);
    if (status != common::Status::Ok()) {
      return status;
    }
    mat_.reset(new TimeOrderAggregateMaterializer(ids_, iters, fn_));
    return common::Status::Ok();
  }

  common::Status extract_result(std::unique_ptr<ColumnMaterializer> *dest) {
    if (!mat_) {
      return common::Status::NoData();
    }
    *dest = std::move(mat_);
    return common::Status::Ok();
  }
};

struct TwoStepQueryPlan : IQueryPlan {
  std::unique_ptr<ProcessingPrelude> prelude_;
  std::unique_ptr<MaterializationStep> mater_;
  std::unique_ptr<ColumnMaterializer> column_;

  template<class T1, class T2>
  TwoStepQueryPlan(T1&& t1, T2&& t2)
        : prelude_(std::forward<T1>(t1))
        , mater_(std::forward<T2>(t2))
  {
  }

  common::Status execute(const ColumnStore &cstore) {
    auto status = prelude_->apply(cstore);
    if (status != common::Status::Ok()) {
      return status;
    }
    status = mater_->apply(prelude_.get());
    if (status != common::Status::Ok()) {
      return status;
    }
    return mater_->extract_result(&column_);
  }

  std::tuple<common::Status, size_t> read(u8 *dest, size_t size) {
    if (!column_) {
      LOG(FATAL) << "Successful execute step required";
    }
    return column_->read(dest, size);
  }
};

// ----------- Query plan builder ------------ //
static bool filtering_enabled(const std::vector<Filter>& flt) {
  bool enabled = false;
  for (const auto& it: flt) {
    enabled |= it.enabled;
  }
  return enabled;
}

static std::tuple<common::Status, std::vector<ValueFilter>> convert_filters(const std::vector<Filter>& fltlist) {
  std::vector<ValueFilter> result;
  for (const auto& filter: fltlist) {
    ValueFilter flt;
    if (filter.flags&Filter::GT) {
      flt.greater_than(filter.gt);
    } else if (filter.flags&Filter::GE) {
      flt.greater_or_equal(filter.ge);
    }
    if (filter.flags&Filter::LT) {
      flt.less_than(filter.lt);
    } else if (filter.flags&Filter::LE) {
      flt.less_or_equal(filter.le);
    }
    if (!flt.validate()) {
      LOG(ERROR) << "Invalid filter";
      return std::make_tuple(common::Status::BadArg(), std::move(result));
    }
    result.push_back(flt);
  }
  return std::make_tuple(common::Status::Ok(), std::move(result));
}

static std::tuple<common::Status, AggregateFilter> convert_aggregate_filter(const Filter& filter, AggregationFunction fun) {
  AggregateFilter aggflt;
  if (filter.enabled) {
    ValueFilter flt;
    if (filter.flags&Filter::GT) {
      flt.greater_than(filter.gt);
    } else if (filter.flags&Filter::GE) {
      flt.greater_or_equal(filter.ge);
    }
    if (filter.flags&Filter::LT) {
      flt.less_than(filter.lt);
    } else if (filter.flags&Filter::LE) {
      flt.less_or_equal(filter.le);
    }
    if (!flt.validate()) {
      LOG(ERROR) << "Invalid filter";
      return std::make_tuple(common::Status::BadArg(), aggflt);
    }
    switch(fun) {
      case AggregationFunction::MIN:
        aggflt.set_filter(AggregateFilter::MIN, flt);
        break;
      case AggregationFunction::MAX:
        aggflt.set_filter(AggregateFilter::MAX, flt);
        break;
      case AggregationFunction::MEAN:
        aggflt.set_filter(AggregateFilter::AVG, flt);
        break;
      case AggregationFunction::SUM:
        LOG(ERROR) << "Aggregation function 'sum' can't be used with the filter";
        return std::make_tuple(common::Status::BadArg(), aggflt);
      case AggregationFunction::CNT:
        LOG(ERROR) << "Aggregation function 'cnt' can't be used with the filter";
        return std::make_tuple(common::Status::BadArg(), aggflt);
      case AggregationFunction::MIN_TIMESTAMP:
      case AggregationFunction::MAX_TIMESTAMP:
        LOG(ERROR) << "Aggregation function 'MIN(MAX)_TIMESTAMP' can't be used with the filter";
        return std::make_tuple(common::Status::BadArg(), aggflt);
      case AggregationFunction::FIRST_TIMESTAMP:
      case AggregationFunction::LAST_TIMESTAMP:
        LOG(ERROR) << "Aggregation function 'FIRST(LAST)_TIMESTAMP' can't be used with the filter";
        return std::make_tuple(common::Status::BadArg(), aggflt);
      case AggregationFunction::FIRST:
      case AggregationFunction::LAST:
        LOG(ERROR) << "Aggregation function 'FIRST(LAST)' can't be used with the filter";
        return std::make_tuple(common::Status::BadArg(), aggflt);
    };
  }
  return std::make_tuple(common::Status::Ok(), aggflt);
}

static std::tuple<common::Status, std::vector<AggregateFilter>> convert_aggregate_filters(const std::vector<Filter>& fltlist,
                                                                                          const std::vector<AggregationFunction>& funclst) {
  std::vector<AggregateFilter> result;
  if (fltlist.size() != funclst.size()) {
    LOG(ERROR) << "Number of filters doesn't match number of columns";
    return std::make_tuple(common::Status::BadArg(), std::move(result));
  }
  AggregateFilter aggflt;
  for (size_t ix = 0; ix < fltlist.size(); ix++) {
    const auto& filter = fltlist[ix];
    if (!filter.enabled) {
      continue;
    }
    AggregationFunction fun = funclst[ix];
    ValueFilter flt;
    if (filter.flags&Filter::GT) {
      flt.greater_than(filter.gt);
    } else if (filter.flags&Filter::GE) {
      flt.greater_or_equal(filter.ge);
    }
    if (filter.flags&Filter::LT) {
      flt.less_than(filter.lt);
    } else if (filter.flags&Filter::LE) {
      flt.less_or_equal(filter.le);
    }
    if (!flt.validate()) {
      LOG(ERROR) << "Invalid filter";
      return std::make_tuple(common::Status::BadArg(), std::move(result));
    }
    switch(fun) {
      case AggregationFunction::MIN:
        aggflt.set_filter(AggregateFilter::MIN, flt);
        continue;
      case AggregationFunction::MAX:
        aggflt.set_filter(AggregateFilter::MAX, flt);
        continue;
      case AggregationFunction::MEAN:
        aggflt.set_filter(AggregateFilter::AVG, flt);
        continue;
      case AggregationFunction::SUM:
        LOG(ERROR) << "Aggregation function 'sum' can't be used with the filter";
        break;
      case AggregationFunction::CNT:
        LOG(ERROR) << "Aggregation function 'cnt' can't be used with the filter";
        break;
      case AggregationFunction::MIN_TIMESTAMP:
      case AggregationFunction::MAX_TIMESTAMP:
        LOG(ERROR) << "Aggregation function 'MIN(MAX)_TIMESTAMP' can't be used with the filter";
        break;
      case AggregationFunction::FIRST_TIMESTAMP:
      case AggregationFunction::LAST_TIMESTAMP:
        LOG(ERROR) << "Aggregation function 'FIRST(LAST)_TIMESTAMP' can't be used with the filter";
        break;
      case AggregationFunction::FIRST:
      case AggregationFunction::LAST:
        LOG(ERROR) << "Aggregation function 'FIRST(LAST)' can't be used with the filter";
        break;
    };
    return std::make_tuple(common::Status::BadArg(), std::move(result));
  }
  result.push_back(aggflt);
  return std::make_tuple(common::Status::Ok(), std::move(result));
}

/**
 * @brief Layout filters to match columns/ids of the query
 * @param req is a reshape request
 * @return vector of filters and status
 */
static std::tuple<common::Status, std::vector<ValueFilter>> layout_filters(const ReshapeRequest& req) {
  std::vector<ValueFilter> result;

  common::Status s;
  std::vector<ValueFilter> flt;
  std::tie(s, flt) = convert_filters(req.select.filters);
  if (s != common::Status::Ok()) {
    // Bad filter in query
    return std::make_tuple(common::Status::BadArg(), std::move(result));
  }

  if (flt.empty()) {
    // Bad filter in query
    LOG(ERROR) << "Reshape request without filter supplied";
    return std::make_tuple(common::Status::BadArg(), std::move(result));
  }

  // We should duplicate the filters to match the layout of queried data
  for (size_t ixrow = 0; ixrow < req.select.columns.at(0).ids.size(); ixrow++) {
    for (size_t ixcol = 0; ixcol < req.select.columns.size(); ixcol++) {
      const auto& rowfilter = flt.at(ixcol);
      result.push_back(rowfilter);
    }
  }
  return std::make_tuple(common::Status::Ok(), std::move(result));
}

static std::tuple<common::Status, std::vector<AggregateFilter>> layout_aggregate_filters(const ReshapeRequest& req) {
  std::vector<AggregateFilter> result;
  AggregateFilter::Mode common_mode = req.select.filter_rule == FilterCombinationRule::ALL
      ? AggregateFilter::Mode::ALL
      : AggregateFilter::Mode::ANY;
  common::Status s;
  std::vector<AggregateFilter> flt;
  std::tie(s, flt) = convert_aggregate_filters(req.select.filters, req.agg.func);
  if (s != common::Status::Ok()) {
    // Bad filter in query
    return std::make_tuple(common::Status::BadArg(), std::move(result));
  }

  if (flt.empty()) {
    // Bad filter in query
    LOG(ERROR) << "Reshape request without filter supplied";
    return std::make_tuple(common::Status::BadArg(), std::move(result));
  }

  // We should duplicate the filters to match the layout of queried data
  for (size_t ixrow = 0; ixrow < req.select.columns.at(0).ids.size(); ixrow++) {
    for (size_t ixcol = 0; ixcol < req.select.columns.size(); ixcol++) {
      auto& colfilter = flt.at(ixcol);
      colfilter.mode = common_mode;
      result.push_back(colfilter);
    }
  }
  return std::make_tuple(common::Status::Ok(), std::move(result));
}

static std::tuple<common::Status, std::vector<AggregateFilter>> layout_aggregate_join_filters(const ReshapeRequest& req) {
  std::vector<AggregateFilter> result;
  AggregateFilter::Mode common_mode = req.select.filter_rule == FilterCombinationRule::ALL
      ? AggregateFilter::Mode::ALL
      : AggregateFilter::Mode::ANY;
  common::Status s;
  if (req.agg.func.size() != 1) {
    // TODO: support N:N downsampling
    return std::make_tuple(common::Status::BadArg(), std::move(result));
  }
  std::vector<AggregateFilter> colfilters;
  auto downsampling_fn = req.agg.func.front();
  for (auto it: req.select.filters) {
    AggregateFilter flt;
    std::tie(s, flt) = convert_aggregate_filter(it, downsampling_fn);
    if (s != common::Status::Ok()) {
      // Bad filter in query
      return std::make_tuple(common::Status::BadArg(), std::move(result));
    }
    flt.mode = common_mode;
    colfilters.push_back(std::move(flt));
  }

  if (colfilters.empty() || colfilters.size() != req.select.columns.size()) {
    // Bad filter in query
    LOG(ERROR) << "Reshape request without filter supplied";
    return std::make_tuple(common::Status::BadArg(), std::move(result));
  }

  // We should duplicate the filters to match the layout of queried data
  for (size_t ixrow = 0; ixrow < req.select.columns.at(0).ids.size(); ixrow++) {
    for (size_t ixcol = 0; ixcol < req.select.columns.size(); ixcol++) {
      auto& colfilter = colfilters.at(ixcol);
      result.push_back(colfilter);
    }
  }

  return std::make_tuple(common::Status::Ok(), std::move(result));
}

static std::tuple<common::Status, std::unique_ptr<IQueryPlan>> scan_query_plan(ReshapeRequest const& req) {
  // Hardwired query plan for scan query
  // Tier1
  // - List of range scan/filter operators
  // Tier2
  // - If group-by is enabled:
  //   - Transform ids and matcher (generate new names)
  //   - Add merge materialization step (series or time order, depending on the
  //     order-by clause.
  // - Otherwise
  //   - If oreder-by is series add chain materialization step.
  //   - Otherwise add merge materializer.

  std::unique_ptr<IQueryPlan> result;

  if (req.agg.enabled || req.select.columns.size() != 1) {
    return std::make_tuple(common::Status::BadArg(), std::move(result));
  }

  std::unique_ptr<ProcessingPrelude> t1stage;
  if (filtering_enabled(req.select.filters)) {
    // Scan query can only have one filter
    common::Status s;
    std::vector<ValueFilter> flt;
    std::tie(s, flt) = layout_filters(req);
    if (s != common::Status::Ok()) {
      return std::make_tuple(common::Status::BadArg(), std::move(result));
    }
    t1stage.reset(new FilterProcessingStep(req.select.begin,
                                           req.select.end,
                                           flt,
                                           req.select.columns.at(0).ids));
  } else {
    t1stage.reset(new ScanProcessingStep  (req.select.begin,
                                           req.select.end,
                                           req.select.columns.at(0).ids));
  }

  std::unique_ptr<MaterializationStep> t2stage;
  if (req.group_by.enabled) {
    std::vector<ParamId> ids;
    for(auto id: req.select.columns.at(0).ids) {
      auto it = req.group_by.transient_map.find(id);
      if (it != req.group_by.transient_map.end()) {
        ids.push_back(it->second);
      }
    }
    if (req.order_by == OrderBy::SERIES) {
      t2stage.reset(new MergeBy<OrderBy::SERIES>(std::move(ids)));
    } else {
      t2stage.reset(new MergeBy<OrderBy::TIME>(std::move(ids)));
    }
  } else {
    auto ids = req.select.columns.at(0).ids;
    if (req.order_by == OrderBy::SERIES) {
      t2stage.reset(new Chain<>(std::move(ids)));
    } else {
      t2stage.reset(new MergeBy<OrderBy::TIME>(std::move(ids)));
    }
  }

  result.reset(new TwoStepQueryPlan(std::move(t1stage), std::move(t2stage)));
  return std::make_tuple(common::Status::Ok(), std::move(result));
}

static std::tuple<common::Status, std::unique_ptr<IQueryPlan>> scan_events_query_plan(ReshapeRequest const& req) {
  // Hardwired query plan for scan query
  // Tier1
  // - List of range scan/filter operators
  // Tier2
  // - If group-by is enabled:
  //   - Transform ids and matcher (generate new names)
  //   - Add merge materialization step (series or time order, depending on the
  //     order-by clause.
  // - Otherwise
  //   - If oreder-by is series add chain materialization step.
  //   - Otherwise add merge materializer.

  std::unique_ptr<IQueryPlan> result;

  if (req.agg.enabled || req.select.columns.size() != 1) {
    return std::make_tuple(common::Status::BadArg(), std::move(result));
  }

  std::unique_ptr<ProcessingPrelude> t1stage;
  if (req.select.event_body_regex.empty()) {
    // Regex filter is not set
    t1stage.reset(new ScanEventsProcessingStep  (req.select.begin,
                                                 req.select.end,
                                                 req.select.columns.at(0).ids));
  } else {
    t1stage.reset(new ScanEventsProcessingStep  (req.select.begin,
                                                 req.select.end,
                                                 req.select.event_body_regex,
                                                 req.select.columns.at(0).ids));
  }

  std::unique_ptr<MaterializationStep> t2stage;
  if (req.group_by.enabled) {
    std::vector<ParamId> ids;
    for(auto id: req.select.columns.at(0).ids) {
      auto it = req.group_by.transient_map.find(id);
      if (it != req.group_by.transient_map.end()) {
        ids.push_back(it->second);
      }
    }
    if (req.order_by == OrderBy::SERIES) {
      t2stage.reset(new MergeBy<OrderBy::SERIES, BinaryDataOperator>(std::move(ids)));
    } else {
      t2stage.reset(new MergeBy<OrderBy::TIME, BinaryDataOperator>(std::move(ids)));
    }
  } else {
    auto ids = req.select.columns.at(0).ids;
    if (req.order_by == OrderBy::SERIES) {
      t2stage.reset(new Chain<BinaryDataOperator>(std::move(ids)));
    } else {
      t2stage.reset(new MergeBy<OrderBy::TIME, BinaryDataOperator>(std::move(ids)));
    }
  }

  result.reset(new TwoStepQueryPlan(std::move(t1stage), std::move(t2stage)));
  return std::make_tuple(common::Status::Ok(), std::move(result));
}

static std::tuple<common::Status, std::unique_ptr<IQueryPlan>> aggregate_query_plan(ReshapeRequest const& req) {
  // Hardwired query plan for aggregate query
  // Tier1
  // - List of aggregate operators
  // Tier2
  // - If group-by is enabled:
  //   - Transform ids and matcher (generate new names)
  //   - Add merge materialization step (series or time order, depending on the
  //     order-by clause.
  // - Otherwise
  //   - If oreder-by is series add chain materialization step.
  //   - Otherwise add merge materializer.

  std::unique_ptr<IQueryPlan> result;

  if (req.order_by == OrderBy::TIME || req.agg.enabled == false || req.agg.step != 0) {
    return std::make_tuple(common::Status::BadArg(), std::move(result));
  }

  std::unique_ptr<ProcessingPrelude> t1stage;
  t1stage.reset(new AggregateProcessingStep(req.select.begin, req.select.end, req.select.columns.at(0).ids));

  std::unique_ptr<MaterializationStep> t2stage;
  if (req.group_by.enabled) {
    std::vector<ParamId> ids;
    for(auto id: req.select.columns.at(0).ids) {
      auto it = req.group_by.transient_map.find(id);
      if (it != req.group_by.transient_map.end()) {
        ids.push_back(it->second);
      }
    }
    t2stage.reset(new AggregateCombiner(std::move(ids), req.agg.func));
  } else {
    auto ids = req.select.columns.at(0).ids;
    t2stage.reset(new Aggregate(std::move(ids), req.agg.func));
  }

  result.reset(new TwoStepQueryPlan(std::move(t1stage), std::move(t2stage)));
  return std::make_tuple(common::Status::Ok(), std::move(result));
}

static std::tuple<common::Status, std::unique_ptr<IQueryPlan>> join_query_plan(ReshapeRequest const& req) {
  std::unique_ptr<IQueryPlan> result;

  // Group-by is not supported currently
  if (req.group_by.enabled || req.select.columns.size() < 2) {
    return std::make_tuple(common::Status::BadArg(), std::move(result));
  }

  if (req.agg.enabled == false) {
    std::unique_ptr<ProcessingPrelude> t1stage;
    std::vector<ParamId> t1ids;
    int cardinality = static_cast<int>(req.select.columns.size());
    for (size_t i = 0; i < req.select.columns.at(0).ids.size(); i++) {
      for (int c = 0; c < cardinality; c++) {
        t1ids.push_back(req.select.columns.at(static_cast<size_t>(c)).ids.at(i));
      }
    }
    if (filtering_enabled(req.select.filters)) {
      // Join query should have many filters (filter per metric)
      common::Status s;
      std::vector<ValueFilter> flt;
      std::tie(s, flt) = layout_filters(req);
      if (s != common::Status::Ok()) {
        return std::make_tuple(common::Status::BadArg(), std::move(result));
      }
      t1stage.reset(new FilterProcessingStep(req.select.begin,
                                             req.select.end,
                                             flt,
                                             std::move(t1ids)));
    } else {
      t1stage.reset(new ScanProcessingStep(req.select.begin, req.select.end, std::move(t1ids)));
    }

    std::unique_ptr<MaterializationStep> t2stage;

    if (req.group_by.enabled) {
      return std::make_tuple(common::Status::BadArg(), std::move(result));
    } else {
      t2stage.reset(new Join(req.select.columns.at(0).ids, cardinality, req.order_by, req.select.begin, req.select.end));
    }

    result.reset(new TwoStepQueryPlan(std::move(t1stage), std::move(t2stage)));
    return std::make_tuple(common::Status::Ok(), std::move(result));
  } else {
    std::unique_ptr<ProcessingPrelude> t1stage;
    std::vector<ParamId> t1ids;
    int cardinality = static_cast<int>(req.select.columns.size());
    for (size_t i = 0; i < req.select.columns.at(0).ids.size(); i++) {
      for (int c = 0; c < cardinality; c++) {
        t1ids.push_back(req.select.columns.at(static_cast<size_t>(c)).ids.at(i));
      }
    }
    if (filtering_enabled(req.select.filters)) {
      // Scan query can only have one filter
      common::Status s;
      std::vector<AggregateFilter> flt;
      std::tie(s, flt) = layout_aggregate_join_filters(req);
      if (s != common::Status::Ok()) {
        return std::make_tuple(common::Status::BadArg(), std::move(result));
      }
      t1stage.reset(new GroupAggregateFilterProcessingStep(req.select.begin,
                                                           req.select.end,
                                                           req.agg.step,
                                                           flt,
                                                           std::move(t1ids),
                                                           req.agg.func.front()));
    } else {
      t1stage.reset(new GroupAggregateProcessingStep(req.select.begin,
                                                     req.select.end,
                                                     req.agg.step,
                                                     std::move(t1ids),
                                                     req.agg.func.front()
                                                    ));
    }

    std::unique_ptr<MaterializationStep> t2stage;

    if (req.group_by.enabled) {
      return std::make_tuple(common::Status::BadArg(), std::move(result));
    } else {
      t2stage.reset(new Join(req.select.columns.at(0).ids, cardinality, req.order_by, req.select.begin, req.select.end));
    }

    result.reset(new TwoStepQueryPlan(std::move(t1stage), std::move(t2stage)));
    return std::make_tuple(common::Status::Ok(), std::move(result));
  }
}

static std::tuple<common::Status, std::unique_ptr<IQueryPlan>> group_aggregate_query_plan(ReshapeRequest const& req) {
  // Hardwired query plan for group aggregate query
  // Tier1
  // - List of group aggregate operators
  // Tier2
  // - If group-by is enabled:
  //   - Transform ids and matcher (generate new names)
  //   - Add merge materialization step (series or time order, depending on the
  //     order-by clause.
  // - Otherwise
  //   - If oreder-by is series add chain materialization step.
  //   - Otherwise add merge materializer.
  std::unique_ptr<IQueryPlan> result;

  if (!req.agg.enabled || req.agg.step == 0) {
    return std::make_tuple(common::Status::BadArg(), std::move(result));
  }

  std::unique_ptr<ProcessingPrelude> t1stage;
  if (filtering_enabled(req.select.filters)) {
    // Scan query can only have one filter
    common::Status s;
    std::vector<AggregateFilter> flt;
    std::tie(s, flt) = layout_aggregate_filters(req);
    if (s != common::Status::Ok()) {
      return std::make_tuple(common::Status::BadArg(), std::move(result));
    }
    t1stage.reset(new GroupAggregateFilterProcessingStep(req.select.begin,
                                                         req.select.end,
                                                         req.agg.step,
                                                         flt,
                                                         req.select.columns.at(0).ids));
  } else {
    t1stage.reset(new GroupAggregateProcessingStep(req.select.begin,
                                                   req.select.end,
                                                   req.agg.step,
                                                   req.select.columns.at(0).ids));
  }

  std::unique_ptr<MaterializationStep> t2stage;
  if (req.group_by.enabled) {
    std::vector<ParamId> ids;
    for(auto id: req.select.columns.at(0).ids) {
      auto it = req.group_by.transient_map.find(id);
      if (it != req.group_by.transient_map.end()) {
        ids.push_back(it->second);
      }
    }
    if (req.order_by == OrderBy::SERIES) {
      t2stage.reset(new GroupAggregateCombiner<OrderBy::SERIES>(std::move(ids), req.agg.func));
    } else {
      t2stage.reset(new GroupAggregateCombiner<OrderBy::TIME>(ids, req.agg.func));
    }
  } else {
    if (req.order_by == OrderBy::SERIES) {
      t2stage.reset(new SeriesOrderAggregate(req.select.columns.at(0).ids, req.agg.func));
    } else {
      t2stage.reset(new TimeOrderAggregate(req.select.columns.at(0).ids, req.agg.func));
    }
  }

  result.reset(new TwoStepQueryPlan(std::move(t1stage), std::move(t2stage)));
  return std::make_tuple(common::Status::Ok(), std::move(result));
}

std::tuple<common::Status, std::unique_ptr<IQueryPlan>> QueryPlanBuilder::create(const ReshapeRequest& req) {
  if (req.agg.enabled && req.agg.step == 0) {
    // Aggregate query
    return aggregate_query_plan(req);
  } else if (req.agg.enabled && req.agg.step != 0) {
    // Group aggregate query
    if (req.select.columns.size() == 1) {
      return group_aggregate_query_plan(req);
    }
    else {
      return join_query_plan(req);
    }
  } else if (req.agg.enabled == false && req.select.columns.size() > 1) {
    // Join query
    return join_query_plan(req);
  } else if (req.select.events) {
    // Select events
    return scan_events_query_plan(req);
  }
  // Select metrics
  return scan_query_plan(req);
}

void QueryPlanExecutor::execute(const storage::ColumnStore& cstore, std::unique_ptr<qp::IQueryPlan>&& iter, qp::IStreamProcessor& qproc) {
  common::Status status = iter->execute(cstore);
  if (status != common::Status::Ok()) {
    LOG(ERROR) << "Query plan error " << status.ToString();
    qproc.set_error(status);
    return;
  }
  const size_t dest_size = 0x1000;
  std::vector<u8> dest;
  dest.resize(dest_size);
  while (status == common::Status::Ok()) {
    size_t size;
    // This is OK because normal query (aggregate or select) will write fixed size samples with size = sizeof(Sample).
    //
    std::tie(status, size) = iter->read(reinterpret_cast<u8*>(dest.data()), dest_size);
    if (status != common::Status::Ok() && (status != common::Status::NoData() && status != common::Status::Unavailable())) {
      LOG(ERROR) << "Iteration error " << status.ToString();
      qproc.set_error(status);
      return;
    }

    size_t pos = 0;
    while(pos < size) {
      Sample const* sample = reinterpret_cast<Sample const*>(dest.data() + pos);
      if (!qproc.put(*sample)) {
        LOG(INFO) << "Iteration stopped by client";
        return;
      }
      pos += sample->payload.size;
    }
  }
}

}  // namespace qp
}  // namespaces faststdb
