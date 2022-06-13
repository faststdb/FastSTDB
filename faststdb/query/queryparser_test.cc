/*!
 * \file queryparser_test.cc
 */
#include "faststdb/query/queryparser.h"

#include "gtest/gtest.h"

#include <sstream>

#include "faststdb/common/datetime.h"
#include "faststdb/index/seriesparser.h"

namespace faststdb {
namespace qp {

std::string make_scan_query(Timestamp begin, Timestamp end, OrderBy order) {
  std::stringstream str;
  str << "{ \"select\": \"test\", \"range\": { \"from\": " << "\"" << DateTimeUtil::to_iso_string(begin) << "\"";
  str << ", \"to\": " << "\"" << DateTimeUtil::to_iso_string(end) << "\"" << "},";
  str << "  \"order-by\": " << (order == OrderBy::SERIES ? "\"series\"" : "\"time\"");
  str << "}";
  return str.str();
}

TEST(TestQueryParser, Test_1) {
  std::string query_json = make_scan_query(1136214245999999999ul, 1136215245999999999ul, OrderBy::TIME); 
  common::Status status;
  boost::property_tree::ptree ptree;
  ErrorMsg error_msg;
  std::tie(status, ptree, error_msg) = QueryParser::parse_json(query_json.c_str());
  EXPECT_TRUE(status.IsOk());

  QueryKind query_kind;
  std::tie(status, query_kind, error_msg) = QueryParser::get_query_kind(ptree);
  EXPECT_TRUE(status.IsOk());
  EXPECT_EQ(qp::QueryKind::SELECT, query_kind);

  SeriesMatcher series_matcher;
  ReshapeRequest req;
  std::tie(status, req, error_msg) = QueryParser::parse_select_query(ptree, series_matcher);
  EXPECT_TRUE(status.IsOk());

  std::vector<std::shared_ptr<Node>> nodes;
  InternalCursor* cur = nullptr;
  std::tie(status, nodes, error_msg) = QueryParser::parse_processing_topology(ptree, cur, req);
  EXPECT_EQ(1, nodes.size());
}

}  // namespace qp
}  // namespace faststdb
