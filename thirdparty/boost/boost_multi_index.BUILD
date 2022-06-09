package(default_visibility = ["//visibility:public"])

licenses(["notice"])

cc_library(
  name = "multi_index",
  includes = [
    "include/",
  ],
  hdrs = glob([
    "include/boost/**/*.hpp",
  ]),
  srcs = [
  ],
  deps = [
    "@com_github_boost_serialization//:serialization",
    "@com_github_boost_tuple//:tuple",
    "@com_github_boost_foreach//:foreach",
  ]
)
