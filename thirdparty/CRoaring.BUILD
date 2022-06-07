licenses(["notice"])

package(default_visibility = ["//visibility:public"])

include_files = [
   "include/roaring/roaring.hh",
]

lib_files = [
    "lib64/libroaring.a",
]

genrule(
    name = "libcroaring-srcs",
    outs = include_files + lib_files,
    cmd = "\n".join([
        'set -x',
        'export INSTALL_DIR=$$(pwd)/$(@D)',
        'export TMP_DIR=$$(mktemp -d -t libcroaring.XXXXX)',
        'mkdir -p $$TMP_DIR',
        'cp -R $$(pwd)/external/CRoaring/* $$TMP_DIR',
        'cd $$TMP_DIR/CRoaring-0.5.0',
        'mkdir build',
        'cd build',
        'cmake ../ -DCMAKE_INSTALL_PREFIX=$$INSTALL_DIR -DCMAKE_INSTALL_LIBDIR=$$INSTALL_DIR/lib64',
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
