package(default_visibility = ["//visibility:public"])

licenses(["notice"])

cc_library(
  name = "atomic",
  includes = [
      "include/",
  ],
  hdrs = glob([
      "include/boost/**/*.hpp",
  ]),
  srcs = [
  ],
  deps = [
      "@com_github_boost_assert//:assert",
      "@com_github_boost_config//:config",
      "@com_github_boost_static_assert//:static_assert",
  ]
)
