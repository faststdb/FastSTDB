package(default_visibility = ["//visibility:public"])

licenses(["notice"])

cc_library(
  name = "chrono",
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
    "@com_github_boost_core//:core",
    "@com_github_boost_system//:system",
    "@com_github_boost_type_traits//:type_traits",
    "@com_github_boost_static_assert//:static_assert",
    "@com_github_boost_ratio//:ratio",
    "@com_github_boost_mpl//:mpl",
    "@com_github_boost_utility//:utility",
    "@com_github_boost_throw_exception//:throw_exception",
 
  ]
)
