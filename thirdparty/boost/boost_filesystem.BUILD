package(default_visibility = ["//visibility:public"])

licenses(["notice"])

cc_library(
  name = "filesystem",
  includes = [
      "include/",
      "src",
  ],
  hdrs = glob([
      "include/boost/**/*.hpp",
      "src/*.hpp",
  ]),
  srcs = glob([
        "src/*.cpp"
  ]),
  copts = [
    "-DBOOST_FILESYSTEM_NO_CXX20_ATOMIC_REF",
  ],
  deps = [
      "@com_github_boost_config//:config",
      "@com_github_boost_detail//:detail",
      "@com_github_boost_functional//:functional",
      "@com_github_boost_io//:io",
      "@com_github_boost_iterator//:iterator",
      "@com_github_boost_smart_ptr//:smart_ptr",
      "@com_github_boost_range//:range",
      "@com_github_boost_system//:system",
      "@com_github_boost_type_traits//:type_traits",
      "@com_github_boost_container_hash//:container_hash",
      "@com_github_boost_atomic//:atomic",
  ]
)
