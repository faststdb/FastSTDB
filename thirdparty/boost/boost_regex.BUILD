package(default_visibility = ["//visibility:public"])

licenses(["notice"])

cc_library(
  name = "heap",
  includes = [
      "include/",
  ],
  hdrs = glob([
      "include/boost/**/*.hpp",
      "include/boost/*.hpp",
      "include/boost/*.h",
  ]),
  srcs = glob([
     "src/*.cpp",
  ]),
  deps = [
      "@com_github_boost_config//:config",
      "@com_github_boost_predef//:predef",
      "@com_github_boost_assert//:assert",
      "@com_github_boost_throw_exception//:throw_exception",
  ]
)
