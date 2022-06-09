package(default_visibility = ["//visibility:public"])

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_proto_library", "cc_test")
load("@rules_proto//proto:defs.bzl", "proto_library")

cc_library(
  name = "index",
  srcs = [
    "invertedindex.h",
    "seriesparser.cc",
    "stringpool.cc",
  ],
  hdrs = [
    "invertedindex.h",
    "seriesparser.h",
    "stringpool.h",
  ],
  alwayslink = 1,
  copts = [
    "-std=c++14",
  ],
  deps = [
    "//faststdb/storage:storage",
    "@com_github_boost_regex//:regex",
  ],
)

