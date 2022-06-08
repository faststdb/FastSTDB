licenses(["notice"])

package(default_visibility = ["//visibility:public"])

include_files = [
   "include/lz4frame.h",
   "include/lz4frame_static.h",
   "include/lz4.h",
   "include/lz4hc.h",
   "include/xxhash.h",
]

lib_files = [
    "lib64/liblz4.a",
]

genrule(
    name = "liblz4-srcs",
    outs = include_files + lib_files,
    cmd = "\n".join([
        'set -x',
        'export INSTALL_DIR=$$(pwd)/$(@D)',
        'export TMP_DIR=$$(mktemp -d -t liblz4.XXXXX)',
        'mkdir -p $$TMP_DIR',
        'cp -R $$(pwd)/external/lz4/* $$TMP_DIR',
        'cd $$TMP_DIR',
        'make',
        'mkdir -p $$INSTALL_DIR/include',
        'mkdir -p $$INSTALL_DIR/lib64',
        'cp $$TMP_DIR/lib/*.h $$INSTALL_DIR/include',
        'cp $$TMP_DIR/lib/*.a $$INSTALL_DIR/lib64',
        'rm -rf $$TMP_DIR',
    ]),
)

cc_library(
    name = "liblz4",
    srcs = lib_files,
    hdrs = include_files,
    includes=["include"],
    linkstatic = 1,
)
