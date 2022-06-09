package(default_visibility = ["//visibility:public"])

licenses(["notice"])

cc_library(
  name = "serialization",
  includes = [
    "include/",
  ],
  hdrs = glob([
    "include/boost/**/*.hpp",
    "src/*.ipp",
  ]),
  srcs = glob([
    "src/*.cpp",
  ]),
  deps = [
    "@com_github_boost_assert//:assert",
    "@com_github_boost_config//:config",
    "@com_github_boost_integer//:integer",
    "@com_github_boost_core//:core",
    "@com_github_boost_mpl//:mpl",
    "@com_github_boost_static_assert//:static_assert",
    "@com_github_boost_smart_ptr//:smart_ptr",
    "@com_github_boost_io//:io",
    "@com_github_boost_move//:move",
    "@com_github_boost_iterator//:iterator",
    "@com_github_boost_spirit//:spirit",
    "@com_github_boost_utility//:utility",
    "@com_github_boost_array//:array",
    "@com_github_boost_function//:function",
  ]
)
