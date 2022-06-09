workspace(name="FastSTDB")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")

git_repository(
    name = "gtest",
    remote = "https://github.com/google/googletest.git",
    branch = "v1.10.x",
)

git_repository(
    name = "glog",
    remote = "https://github.com/google/glog.git",
    tag = "v0.4.0",
)

git_repository(
    name = "com_github_gflags_gflags",
    remote = "https://github.com/gflags/gflags.git",
    tag = "v2.2.2",
)

git_repository(
   name = "absl",
   remote = "git@github.com:abseil/abseil-cpp.git",
   tag = "20211102.0",
)

http_archive(
    name = "rules_proto",
    sha256 = "66bfdf8782796239d3875d37e7de19b1d94301e8972b3cbd2446b332429b4df1",
    strip_prefix = "rules_proto-4.0.0",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/rules_proto/archive/refs/tags/4.0.0.tar.gz",
        "https://github.com/bazelbuild/rules_proto/archive/refs/tags/4.0.0.tar.gz",
    ],
)

load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")
rules_proto_dependencies()
rules_proto_toolchains()

http_archive(
   name = "CRoaring",
   sha256 = "edab1b1a464e5a361ff622dc833170b2f33729c161aee4c2e53a324ac62ef78f",
   urls = [
     "https://github.com/RoaringBitmap/CRoaring/archive/refs/tags/v0.5.0.tar.gz",
   ],
   build_file = "@//thirdparty:CRoaring.BUILD",
)

load("//thirdparty:deps.bzl", "boost_deps")
boost_deps()

new_git_repository(
	name = "robin-map",
	build_file = "@//thirdparty:robin-map.BUILD",
	tag = "v1.0.1",
	remote = "https://github.com/Tessil/robin-map.git",
)

new_git_repository(
	name = "lz4",
	build_file = "@//thirdparty:lz4.BUILD",
	tag = "v1.9.3",
	remote = "https://github.com/lz4/lz4.git",
)

new_git_repository(
	name = "apr",
	build_file = "@//thirdparty:apr.BUILD",
	branch = "trunk",
  remote = "https://github.com/faststdb/apr.git",
)

new_git_repository(
	name = "sqlite",
	build_file = "@//thirdparty:sqlite.BUILD",
	tag = "version-3.38.5",
  remote = "https://github.com/sqlite/sqlite.git",
)

new_git_repository(
	name = "expat",
	build_file = "@//thirdparty:expat.BUILD",
	tag = "R_2_4_8",
  remote = "https://github.com/libexpat/libexpat.git",
)

new_git_repository(
	name = "muparser",
	build_file = "@//thirdparty:muparser.BUILD",
	tag = "v2.3.2",
  remote = "https://github.com/beltoforion/muparser.git",
)
