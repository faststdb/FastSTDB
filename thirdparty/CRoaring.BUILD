licenses(["notice"])

package(default_visibility = ["//visibility:public"])

include_files = [
   "include/roaring/*.h",
   "include/roaring/**/*.h",
]

lib_files = [
    "src/libroaring.a",
]

genrule(
    name = "libcroaring-srcs",
    outs = include_files + lib_files,
    cmd = "\n".join([
        'export INSTALL_DIR=$$(pwd)/$(@D)',
        'export TMP_DIR=$$(mktemp -d -t libzmq.XXXXX)',
        'mkdir -p $$TMP_DIR',
        'cp -R $$(pwd)/../../../../../external/libzmq/* $$TMP_DIR',
        'cd $$TMP_DIR',
        'mkdir build',
        'cd build',
        'cmake ../ -DCMAKE_INSTALL_PREFIX=$$INSTALL_DIR',
        'make install',
        'rm -rf $$TMP_DIR',
    ]),
)

cc_library(
    name = "libcroaring",
    srcs = lib_files,
    hdrs = include_files,
    includes=["include"],
    linkstatic = 1,
)
