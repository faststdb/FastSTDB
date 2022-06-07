load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def boost_deps():
  http_archive(
    name = "com_github_boost_array",
    sha256 = "a3abe63ca22a3c2de09cd2bf4dbef9c3c87897c11bef8f26cb35ad39f164d7ce",
    build_file = "@//thirdparty/boost:boost_array.BUILD",
    urls = ["https://github.com/boostorg/array/archive/refs/tags/boost-1.79.0.tar.gz"],
  )

  http_archive(
    name = "com_github_boost_config",
    build_file = "@//thirdparty/boost:boost_config.BUILD",
    urls = ["https://github.com/boostorg/config/archive/refs/tags/boost-1.79.0.tar.gz"],
  )

  http_archive(
    name = "com_github_boost_detail",
    build_file = "@//thirdparty/boost:boost_detail.BUILD",
    urls = ["https://github.com/boostorg/detail/archive/refs/tags/boost-1.79.0.tar.gz"],
  )

  http_archive(
    name = "com_github_boost_core",
    build_file = "@//thirdparty/boost:boost_core.BUILD",
    urls = ["https://github.com/boostorg/core/archive/refs/tags/boost-1.79.0.tar.gz"],
  )

  new_git_repository(
    name = "com_github_boost_throw_exception",
    build_file = "@//thirdparty/boost:boost_throw_exception.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/throw_exception.git",
  )

  new_git_repository(
    name = "com_github_boost_heap",
    build_file = "@//thirdparty/boost:boost_heap.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/heap.git",
  )

  new_git_repository(
    name = "com_github_boost_static_assert",
    build_file = "@//thirdparty/boost:boost_static_assert.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/static_assert.git",
  )

  new_git_repository(
    name = "com_github_boost_assert",
    build_file = "@//thirdparty/boost:boost_assert.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/assert.git",
  )

  new_git_repository(
    name = "com_github_boost_asio",
    build_file = "@//thirdparty/boost:boost_asio.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/asio.git",
  )

  new_git_repository(
    name = "com_github_boost_algorithm",
    build_file = "@//thirdparty/boost:boost_algorithm.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/algorithm.git",
  )

  new_git_repository(
    name = "com_github_boost_bind",
    build_file = "@//thirdparty/boost:boost_bind.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/bind.git",
  )

  new_git_repository(
    name = "com_github_boost_concept_check",
    build_file = "@//thirdparty/boost:boost_concept_check.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/concept_check.git",
  )

  new_git_repository(
    name = "com_github_boost_container",
    build_file = "@//thirdparty/boost:boost_container.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/container.git",
  )

  new_git_repository(
    name = "com_github_boost_crc",
    build_file = "@//thirdparty/boost:boost_crc.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/crc.git",
  )

  new_git_repository(
    name = "com_github_boost_date_time",
    build_file = "@//thirdparty/boost:boost_date_time.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/date_time.git",
  )

  new_git_repository(
    name = "com_github_boost_filesystem",
    build_file = "@//thirdparty/boost:boost_filesystem.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/filesystem.git",
  )

  new_git_repository(
    name = "com_github_boost_format",
    build_file = "@//thirdparty/boost:boost_format.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/format.git",
  )

  new_git_repository(
    name = "com_github_boost_function",
    build_file = "@//thirdparty/boost:boost_function.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/function.git",
  )

  new_git_repository(
    name = "com_github_boost_functional",
    build_file = "@//thirdparty/boost:boost_functional.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/functional.git",
  )

  new_git_repository(
    name = "com_github_boost_fusion",
    build_file = "@//thirdparty/boost:boost_fusion.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/fusion.git",
  )

  new_git_repository(
    name = "com_github_boost_hana",
    build_file = "@//thirdparty/boost:boost_hana.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/hana.git",
  )

  new_git_repository(
    name = "com_github_boost_integer",
    build_file = "@//thirdparty/boost:boost_integer.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/integer.git",
  )

  new_git_repository(
    name = "com_github_boost_io",
    build_file = "@//thirdparty/boost:boost_io.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/io.git",
  )

  new_git_repository(
    name = "com_github_boost_interprocess",
    build_file = "@//thirdparty/boost:boost_interprocess.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/interprocess.git",
  )

  new_git_repository(
    name = "com_github_boost_iterator",
    build_file = "@//thirdparty/boost:boost_iterator.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/iterator.git",
  )

  new_git_repository(
    name = "com_github_boost_intrusive",
    build_file = "@//thirdparty/boost:boost_intrusive.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/intrusive.git",
  )

  new_git_repository(
    name = "com_github_boost_lexical_cast",
    build_file = "@//thirdparty/boost:boost_lexical_cast.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/lexical_cast.git",
  )

  new_git_repository(
    name = "com_github_boost_math",
    build_file = "@//thirdparty/boost:boost_math.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/math.git",
  )

  new_git_repository(
    name = "com_github_boost_move",
    build_file = "@//thirdparty/boost:boost_move.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/move.git",
  )

  new_git_repository(
    name = "com_github_boost_mpl",
    build_file = "@//thirdparty/boost:boost_mpl.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/mpl.git",
  )

  new_git_repository(
    name = "com_github_boost_numeric_conversion",
    build_file = "@//thirdparty/boost:boost_numeric_conversion.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/numeric_conversion.git",
  )

  new_git_repository(
    name = "com_github_boost_optional",
    build_file = "@//thirdparty/boost:boost_optional.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/optional.git",
  )

  new_git_repository(
    name = "com_github_boost_parameter",
    build_file = "@//thirdparty/boost:boost_parameter.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/parameter.git",
  )

  new_git_repository(
    name = "com_github_boost_pool",
    build_file = "@//thirdparty/boost:boost_pool.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/pool.git",
  )

  new_git_repository(
    name = "com_github_boost_predef",
    build_file = "@//thirdparty/boost:boost_predef.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/predef.git",
  )

  new_git_repository(
    name = "com_github_boost_preprocessor",
    build_file = "@//thirdparty/boost:boost_preprocessor.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/preprocessor.git",
  )

  new_git_repository(
    name = "com_github_boost_process",
    build_file = "@//thirdparty/boost:boost_process.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/process.git",
  )

  new_git_repository(
    name = "com_github_boost_random",
    build_file = "@//thirdparty/boost:boost_random.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/random.git",
  )

  new_git_repository(
    name = "com_github_boost_range",
    build_file = "@//thirdparty/boost:boost_range.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/range.git",
  )

  new_git_repository(
    name = "com_github_boost_smart_ptr",
    build_file = "@//thirdparty/boost:boost_smart_ptr.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/smart_ptr.git",
  )

  new_git_repository(
    name = "com_github_boost_system",
    build_file = "@//thirdparty/boost:boost_system.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/system.git",
  )

  new_git_repository(
    name = "com_github_boost_tokenizer",
    build_file = "@//thirdparty/boost:boost_tokenizer.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/tokenizer.git",
  )

  new_git_repository(
    name = "com_github_boost_type_index",
    build_file = "@//thirdparty/boost:boost_type_index.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/type_index.git",
  )

  new_git_repository(
    name = "com_github_boost_type_traits",
    build_file = "@//thirdparty/boost:boost_type_traits.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/type_traits.git",
  )

  new_git_repository(
    name = "com_github_boost_utility",
    build_file = "@//thirdparty/boost:boost_utility.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/utility.git",
  )

  new_git_repository(
    name = "com_github_boost_uuid",
    build_file = "@//thirdparty/boost:boost_uuid.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/uuid.git",
  )

  new_git_repository(
    name = "com_github_boost_variant",
    build_file = "@//thirdparty/boost:boost_variant.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/variant.git",
  )

  new_git_repository(
    name = "com_github_boost_winapi",
    build_file = "@//thirdparty/boost:boost_winapi.BUILD",
    tag = "boost-1.79.0",
    remote = "https://github.com/boostorg/winapi.git",
  )