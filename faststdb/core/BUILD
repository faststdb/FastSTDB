package(default_visibility = ["//visibility:public"])

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_proto_library", "cc_test")
load("@rules_proto//proto:defs.bzl", "proto_library")

cc_library(
  name = "core",
  srcs = [
    "faststdb.cc",
    "cursor.cc",
    "metadatastorage.cc",
    "storage.cc",
  ],
  hdrs = [
    "faststdb.h",
    "cursor.h",
    "metadatastorage.h",
    "storage.h",
  ],
  alwayslink = 1,
  copts = [
    "-std=c++14",
    "-fopenmp",
  ],
  linkopts = [
    "-ldl",
  ],
  deps = [
    "//faststdb/storage:storage",
    "//faststdb/index:index",
    "//faststdb/query:query",
    "@com_github_boost_thread//:thread",
    "@sqlite//:libsqlite",
  ],
)

cc_test(
  name = "metadatastorage_test",
  srcs = ["metadatastorage_test.cc"],
  deps = [
    "@gtest//:gtest",
    "@gtest//:gtest_main",
    ":core",
  ],
)
