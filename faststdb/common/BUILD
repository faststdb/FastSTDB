package(default_visibility = ["//visibility:public"])

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_proto_library", "cc_test")

cc_library(
  name = "common",
  srcs = [
    "crc32c.cc",
    "hash.cc",
    "mmapfile.cc",
    "proto_configure.cc",
    "rwlock.cc",
  ],
  hdrs = [
    "basic.h",
    "blocking_queue.h",
    "crc32c.h",
    "file_utils.h",
    "hash.h",
    "logging.h",
    "mmapfile.h",
    "proto_configure.h",
    "rwlock.h",
    "status.h",
    "singleton.h",
    "thread_local.h",
    "timer.h",
  ],
  alwayslink = 1,
  deps = [
    "@glog",
    "@com_google_protobuf//:protobuf",
  ],
)

cc_test(
  name = "status_test",
  srcs = ["status_test.cc"],
  deps = [
    "@gtest//:gtest",
    "@gtest//:gtest_main",
    ":common",
  ],
)

cc_test(
  name = "rwlock_test",
  srcs = ["rwlock_test.cc"],
  deps = [
    "@gtest//:gtest",
    "@gtest//:gtest_main",
    ":common",
  ],
)

cc_test(
  name = "hash_test",
  srcs = ["hash_test.cc"],
  deps = [
    "@gtest//:gtest",
    "@gtest//:gtest_main",
    ":common",
  ],
)

