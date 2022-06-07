package(default_visibility = ["//visibility:public"])

licenses(["notice"])

cc_library(
  name = "date_time",
  includes = [
    "include/",
  ],
  hdrs = glob([
    "include/boost/**/*.h",
    "include/boost/**/*.hpp",
    "include/boost/**/*.ipp",
  ]),
    linkopts = ["-pthread", "-lrt"],
  srcs = glob([
    "src/*.cpp"
  ]),
  deps = [
    "@com_github_boost_core//:core",
    "@com_github_boost_mpl//:mpl",
    "@com_github_boost_smart_ptr//:smart_ptr",
    "@com_github_boost_utility//:utility",
    "@com_github_boost_static_assert//:static_assert",
    "@com_github_boost_assert//:assert",
    "@com_github_boost_throw_exception//:throw_exception",
    "@com_github_boost_config//:config",
    "@com_github_boost_io//:io",
    "@com_github_boost_algorithm//:algorithm",
    "@com_github_boost_lexical_cast//:lexical_cast",
    "@com_github_boost_tokenizer//:tokenizer",
    "@com_github_boost_regex//:regex",
  ]
)
