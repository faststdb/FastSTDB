package(default_visibility = ["//visibility:public"])

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_proto_library", "cc_test")
load("@rules_proto//proto:defs.bzl", "proto_library")

cc_library(
  name = "query",
  srcs = [
    "queryparser.cc",
    "queryplan.cc",
    "queryprocessor.cc",
    "queryprocessor_framework.cc",
    "query_processing/absolute.cc", 
    "query_processing/eval.cc", 
    "query_processing/filterbyid.cc",
    "query_processing/limiter.cc",
    "query_processing/math.cc",
    "query_processing/rate.cc",
    "query_processing/scale.cc",
    "query_processing/sliding_window.cc",
    "query_processing/spacesaver.cc",
    "query_processing/top.cc",
  ],
  hdrs = [
    "queryparser.h",
    "queryplan.h",
    "external_cursor.h",
    "internal_cursor.h",
    "queryprocessor.h",
    "queryprocessor_framework.h",
    "query_processing/absolute.h", 
    "query_processing/eval.h",
    "query_processing/filterbyid.h",
    "query_processing/limiter.h",
    "query_processing/math.h",
    "query_processing/rate.h",
    "query_processing/scale.h",
    "query_processing/sliding_window.h",
    "query_processing/spacesaver.h",
    "query_processing/top.h",
  ],
  alwayslink = 1,
  copts = [
    "-std=c++14",
  ],
  deps = [
    "//faststdb/storage:storage",
    "//faststdb/index:index",
    "@muparser//:libmuparser",
    "@com_github_boost_property_tree//:property_tree",
    "@com_github_boost_circular_buffer//:circular_buffer",
    "@com_github_boost_exception//:exception",
  ],
)

