licenses(["notice"])

package(default_visibility = ["//visibility:public"])

include_files = [
   "include/tsl/robin_map.h",
]

genrule(
    name = "librobin-map-srcs",
    outs = include_files,
    cmd = "\n".join([
        'set -x',
        'export INSTALL_DIR=$$(pwd)/$(@D)',
        'export TMP_DIR=$$(mktemp -d -t librobin-map.XXXXX)',
        'mkdir -p $$TMP_DIR',
        'cp -R $$(pwd)/external/robin-map/* $$TMP_DIR',
        'cd $$TMP_DIR/robin-map',
        'mkdir build',
        'cd build',
        'cmake ../ -DCMAKE_INSTALL_PREFIX=$$INSTALL_DIR',
        'make install',
        'rm -rf $$TMP_DIR',
    ]),
)

cc_library(
    name = "librobin-map",
    srcs = [],
    hdrs = include_files,
    includes=["include"],
    linkstatic = 1,
)
