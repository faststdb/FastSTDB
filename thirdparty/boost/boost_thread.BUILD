package(default_visibility = ["//visibility:public"])

licenses(["notice"])

cc_library(
  name = "thread",
  includes = [
    "include/",
  ],
  hdrs = glob([
    "include/boost/**/*.hpp",
    "src/pthread/once_atomic.cpp",
  ]),
  srcs = glob([
     "src/pthread/once.cpp",
     "src/pthread/thread.cpp",
     "src/*.cpp",
  ]),
  deps = [
    "@com_github_boost_optional//:optional",
    "@com_github_boost_config//:config",
    "@com_github_boost_core//:core",
    "@com_github_boost_system//:system",
    "@com_github_boost_type_traits//:type_traits",
    "@com_github_boost_static_assert//:static_assert",
    "@com_github_boost_move//:move",
    "@com_github_boost_bind//:bind",
    "@com_github_boost_date_time//:date_time",
    "@com_github_boost_atomic//:atomic",
    "@com_github_boost_chrono//:chrono",
    "@com_github_boost_tuple//:tuple",
    "@com_github_boost_exception//:exception",
  ]
)
