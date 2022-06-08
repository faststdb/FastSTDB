package(default_visibility = ["//visibility:public"])

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_proto_library", "cc_test")
load("@rules_proto//proto:defs.bzl", "proto_library")

proto_library(
  name = "meta_volume_proto",
  srcs = ["meta_volume.proto"],
)

cc_proto_library(
   name = "meta_volume_cc_proto",
   deps = [":meta_volume_proto"],
)


cc_library(
  name = "storage",
  srcs = [
    "block_store.cc",
    "compression.cc",
    "column_store.cc",
    "input_log.cc",
    "meta_storage.cc",
    "nbtree.cc",
    "volume.cc",
    "operators/operator.cc",
    "operators/scan.cc",
    "operators/merge.cc",
    "operators/join.cc",
    "operators/aggregate.cc",
  ],
  hdrs = [
    "block_store.h",
    "compression.h",
    "column_store.h",
    "input_log.h",
    "meta_storage.h",
    "nbtree.h",
    "nbtree_def.h",
    "tuples.h",
    "volume_registry.h",
    "volume.h",
    "operators/operator.h",
    "operators/scan.h",
    "operators/merge.h",
    "operators/join.h",
    "operators/aggregate.h",
  ],
  alwayslink = 1,
  copts = [
    "-std=c++14",
  ],
  deps = [
    "@sqlite//:libsqlite",
    "@expat//:libexpat",
    "@apr//:libapr",
    "@lz4//:liblz4",
    "@CRoaring//:libcroaring",
    "@com_github_boost_range//:range",
    "@com_github_boost_array//:array",
    "@com_github_boost_heap//:heap",
    "@com_github_boost_filesystem//:filesystem",
    "@com_github_boost_container_hash//:container_hash",
    "@com_github_boost_variant//:variant",
    "//faststdb/common:common",
    ":meta_volume_cc_proto",
  ],
)

cc_test(
  name = "meta_storage_test",
  srcs = ["meta_storage_test.cc"],
  deps = [
    "@gtest//:gtest",
    "@gtest//:gtest_main",
    ":storage",
  ],
)

cc_test(
  name = "volume_test",
  srcs = ["volume_test.cc"],
  deps = [
    "@gtest//:gtest",
    "@gtest//:gtest_main",
    ":storage",
  ],
)

cc_test(
  name = "compression_test",
  srcs = ["compression_test.cc"],
  deps = [
    "@gtest//:gtest",
    "@gtest//:gtest_main",
    ":storage",
  ],
)

cc_test(
  name = "block_store_test",
  srcs = ["block_store_test.cc"],
  deps = [
    "@gtest//:gtest",
    "@gtest//:gtest_main",
    ":storage",
  ],
)

cc_test(
  name = "nbtree_test",
  srcs = ["nbtree_test.cc"],
  deps = [
    "@gtest//:gtest",
    "@gtest//:gtest_main",
    ":storage",
  ],
)

cc_test(
  name = "column_store_test",
  srcs = ["column_store_test.cc"],
  deps = [
    "@gtest//:gtest",
    "@gtest//:gtest_main",
    ":storage",
  ],
)

cc_test(
  name = "input_log_test",
  srcs = ["input_log_test.cc"],
  deps = [
    "@gtest//:gtest",
    "@gtest//:gtest_main",
    ":storage",
  ],
)

