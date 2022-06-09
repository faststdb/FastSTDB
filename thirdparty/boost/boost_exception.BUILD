package(default_visibility = ["//visibility:public"])

licenses(["notice"])

cc_library(
  name = "exception",
  includes = [
    "include/",
  ],
  hdrs = glob([
    "include/boost/**/*.hpp",
  ]),
  srcs = glob([
    "src/*.cpp",
  ]),
  deps = [
    "@com_github_boost_config//:config",
  ]
)
