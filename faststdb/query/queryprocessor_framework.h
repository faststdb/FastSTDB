/*!
 * \file queryprocessing_framework.h
 */
#ifndef FASTSTDB_QUERY_QUERYPROCESSING_FRAMEWORK_H_
#define FASTSTDB_QUERY_QUERYPROCESSING_FRAMEWORK_H_

#include <memory>
#include <stdexcept>

#include "faststdb/common/basic.h"
#include "faststdb/index/seriesparser.h"
#include "faststdb/storage/operators/operator.h"

#include <boost/property_tree/ptree.hpp>

namespace faststdb {
namespace qp {

/* ColumnStore + reshape functionality
 *
 * selct cpu where host=XXXX group by tag order by time from 0 to 100;
 * TS  Series name Value
 *  0  cpu tag=Foo    10
 *  0  cpu tag=Bar    20
 *  1  cpu tag=Foo    10
 *  2  cpu tag=Foo    12
 *  2  cpu tag=Bar    30
 *  ...
 *
 * selct cpu where host=XXXX group by tag order by series from 0 to 100;
 * TS  Series name Value
 *  0  cpu tag=Foo    21
 *  1  cpu tag=Foo    20
 * ...
 * 99  cpu tag=Foo    19
 *  0  cpu tag=Bar    20
 *  1  cpu tag=Bar    11
 * ...
 * 99  cpu tag=Bar    14
 *  ...
 *
 * It is possible to add processing steps via IQueryProcessor.
 */
using AggregationFunction = storage::AggregationFunction;

struct Aggregation {
  bool enabled;
  std::vector<AggregationFunction> func;
  u64 step;  // 0 if group by time disabled

  static std::string to_string(AggregationFunction f) {
    switch(f) {
      case AggregationFunction::SUM:
        return "sum";
      case AggregationFunction::CNT:
        return "count";
      case AggregationFunction::MAX:
        return "max";
      case AggregationFunction::MAX_TIMESTAMP:
        return "max_timestamp";
      case AggregationFunction::MEAN:
        return "mean";
      case AggregationFunction::MIN:
        return "min";
      case AggregationFunction::MIN_TIMESTAMP:
        return "min_timestamp";
      case AggregationFunction::LAST:
        return "last";
      case AggregationFunction::FIRST:
        return "first";
      case AggregationFunction::LAST_TIMESTAMP:
        return "last_timestamp";
      case AggregationFunction::FIRST_TIMESTAMP:
        return "first_timestamp";
    };
    LOG(FATAL) << "Invalid aggregation function";
  }

  static std::tuple<common::Status, AggregationFunction> from_string(std::string str) {
    if (str == "min") {
      return std::make_tuple(common::Status::Ok(), AggregationFunction::MIN);
    } else if (str == "max") {
      return std::make_tuple(common::Status::Ok(), AggregationFunction::MAX);
    } else if (str == "sum") {
      return std::make_tuple(common::Status::Ok(), AggregationFunction::SUM);
    } else if (str == "count") {
      return std::make_tuple(common::Status::Ok(), AggregationFunction::CNT);
    } else if (str == "min_timestamp") {
      return std::make_tuple(common::Status::Ok(), AggregationFunction::MIN_TIMESTAMP);
    } else if (str == "max_timestamp") {
      return std::make_tuple(common::Status::Ok(), AggregationFunction::MAX_TIMESTAMP);
    } else if (str == "mean") {
      return std::make_tuple(common::Status::Ok(), AggregationFunction::MEAN);
    } else if (str == "last") {
      return std::make_tuple(common::Status::Ok(), AggregationFunction::LAST);
    } else if (str == "first") {
      return std::make_tuple(common::Status::Ok(), AggregationFunction::FIRST);
    } else if (str == "last_timestamp") {
      return std::make_tuple(common::Status::Ok(), AggregationFunction::LAST_TIMESTAMP);
    } else if (str == "first_timestamp") {
      return std::make_tuple(common::Status::Ok(), AggregationFunction::FIRST_TIMESTAMP);
    }
    return std::make_tuple(common::Status::BadArg(), AggregationFunction::CNT);
  }
};

struct Column {
  std::vector<ParamId> ids;
};

enum class FilterCombinationRule {
  ALL,
  ANY,
};

struct Filter {
  enum {
    GT = 1 << 0,
    LT = 1 << 1,
    GE = 1 << 2,
    LE = 1 << 3,
  };
  bool enabled;
  int    flags;
  double    gt;
  double    lt;
  double    ge;
  double    le;
};

//! Set of ids returned by the query (defined by select and where clauses)
struct Selection {
  //! Set of columns returned by the query (1 columns - select statement, N columns - join statement)
  std::vector<Column>        columns;
  std::vector<Filter>        filters;
  FilterCombinationRule  filter_rule;
  Timestamp                begin;
  Timestamp                  end;
  bool                        events;
  std::string       event_body_regex;

  //! This matcher should be used by Join-statement
  std::shared_ptr<PlainSeriesMatcher>  matcher;
  const SeriesMatcherBase             *global_matcher;

  // NOTE: when using Join stmt, output will contain n-tuples (n is a number of columns used).
  // The samples will have ids from first column but textual representation should be different
  // thus we need to use another matcher. Series names should have the following form:
  // "column1:column2:column3 tag1=val1 tag2=val2 .. tagn=valn
};

//! Mapping from persistent series names to transient series names
struct GroupBy {
  bool enabled;
  std::unordered_map<ParamId, ParamId> transient_map;
};

//! Output order
enum class OrderBy {
  SERIES = 0,
  TIME,
};

//! Reshape request defines what should be sent to query processor
struct ReshapeRequest {
  Aggregation  agg;
  Selection select;
  GroupBy group_by;
  OrderBy order_by;
};


//! Exception triggered by query parser
struct QueryParserError : std::runtime_error {
  QueryParserError(const char* parser_message)
      : std::runtime_error(parser_message) {}
  QueryParserError(std::string parser_message)
      : std::runtime_error(std::move(parser_message)) {}
};

struct Node;

struct MutableSample {
  static constexpr size_t MAX_PAYLOAD_SIZE = sizeof(double) * 58;
  static constexpr size_t MAX_SIZE = 1024 + sizeof(Sample);
  union Payload {
    Sample sample;
    char       raw[MAX_SIZE];
  };
  Payload        payload_;
  u32            size_;
  u32            bitmap_;
  const bool     istuple_;
  const Sample *orig_;

  MutableSample(const Sample* source);

  u32 size() const;

  /** Collapse tuple to single value, the value will be allocated
   * and set to 0. This will be used by functions that produces a
   * single value out of tuple (e.g. sum).
   */
  void collapse();

  double* operator[] (u32 index);

  const double* operator[] (u32 index) const;

  Timestamp get_timestamp() const;

  ParamId get_paramid() const;

  void convert_to_sax_word(u32 width);

  char* get_payload();
};

struct Node {

  virtual ~Node() = default;

  //! Complete adding values
  virtual void complete() = 0;

  /** Process value, return false to interrupt process.
   * Empty sample can be sent to flush all updates.
   */
  virtual bool put(MutableSample& sample) = 0;

  virtual void set_error(common::Status status) = 0;

  // Query validation

  enum QueryFlags {
    EMPTY             = 0,
    GROUP_BY_REQUIRED = 1,
    TERMINAL          = 2,
  };

  /** This method returns set of flags that describes its functioning.
  */
  virtual int get_requirements() const = 0;
};


/**
 * @brief Key hash that can be used in processing functions (aka Nodes)
 * Most of the time the function state is accessed via u64:u32 tuple (id:index).
 */
struct KeyHash : public std::unary_function<std::tuple<ParamId, u32>, std::size_t> {
  typedef std::tuple<ParamId, u32> Key;

  static void hash_combine(std::size_t& seed, u32 v) {
    seed ^= std::hash<u32>()(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
  }

  std::size_t operator()(Key value) const {
    size_t seed = std::hash<ParamId>()(std::get<0>(value));
    hash_combine(seed, std::get<1>(value));
    return seed;
  }
};

/**
 * @brief Key comparator that can be used in processing functions (aka Nodes)
 * Most of the time the function state is accessed via u64:u32 tuple (id:index).
 */
struct KeyEqual : public std::binary_function<std::tuple<ParamId, u32>, std::tuple<ParamId, u32>, bool> {
  typedef std::tuple<ParamId, u32> Key;
  bool operator()(const Key& lhs, const Key& rhs) const {
    return lhs == rhs;
  }
};


struct NodeException : std::runtime_error {
  NodeException(const char* msg)
      : std::runtime_error(msg) {}
};

//! Stream processor interface
struct IStreamProcessor {

  virtual ~IStreamProcessor() = default;

  /** Will be called before query execution starts.
   * If result already obtained - return False.
   * In this case `stop` method shouldn't be called
   * at the end.
   */
  virtual bool start() = 0;

  //! Get new value
  virtual bool put(const Sample& sample) = 0;

  //! Will be called when processing completed without errors
  virtual void stop() = 0;

  //! Will be called on error
  virtual void set_error(common::Status error) = 0;
};


struct BaseQueryParserToken {
  virtual std::shared_ptr<Node> create(boost::property_tree::ptree const& ptree,
                                       const ReshapeRequest&              req,
                                       std::shared_ptr<Node>              next) const = 0;
  virtual std::string get_tag() const                                                 = 0;
};

//! Register QueryParserToken
void add_queryparsertoken_to_registry(BaseQueryParserToken const* ptr);

std::vector<std::string> list_query_registry();

//! Create new node using token registry
std::shared_ptr<Node> create_node(std::string tag, boost::property_tree::ptree const& ptree,
                                  const ReshapeRequest &req,
                                  std::shared_ptr<Node> next);

/** Register new query type
 * NOTE: Each template instantination should be used only once, to create query parser for type.
 */
template <class Target> struct QueryParserToken : BaseQueryParserToken {
  std::string tag;
  
  QueryParserToken(const char* tag)
    : tag(tag) {
      add_queryparsertoken_to_registry(this);
  }
  virtual std::string           get_tag() const { return tag; }
  virtual std::shared_ptr<Node> create(boost::property_tree::ptree const& ptree,
                                       const ReshapeRequest&              req,
                                       std::shared_ptr<Node>              next) const {
    return std::make_shared<Target>(ptree, req, next);
  }
};

}  // namespace qp
}  // namespace faststdb

#endif  // FASTSTDB_QUERY_QUERYPROCESSING_FRAMEWORK_H_
