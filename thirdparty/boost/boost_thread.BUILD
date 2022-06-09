package(default_visibility = ["//visibility:public"])

licenses(["notice"])

cc_library(
  name = "thread",
  includes = [
    "include/",
  ],
  hdrs = glob([
    "include/boost/**/*.hpp",
  ]),
  srcs = glob([
     "src/pthread/*.cpp",
     "src/*.cpp",
  ]),
  deps = [
    "@com_github_boost_optional//:optional",
    "@com_github_boost_config//:config",
  ]
)
