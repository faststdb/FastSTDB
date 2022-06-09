package(default_visibility = ["//visibility:public"])

licenses(["notice"])

cc_library(
  name = "property_tree",
  includes = [
      "include/",
  ],
  hdrs = glob([
      "include/boost/**/*.hpp",
  ]),
  srcs = [
  ],
  deps = [
    "@com_github_boost_optional//:optional",
    "@com_github_boost_any//:any",
    "@com_github_boost_multi_index//:multi_index",
  ]
)
