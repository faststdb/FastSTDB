licenses(["notice"])

package(default_visibility = ["//visibility:public"])

include_files = [
   "include/sqlite3.h",
   "include/sqlite3ext.h",
]

lib_files = [
    "lib/libsqlite3.a",
]

genrule(
    name = "libsqlite-srcs",
    outs = include_files + lib_files,
    cmd = "\n".join([
        'set -x',
        'export INSTALL_DIR=$$(pwd)/$(@D)',
        'export TMP_DIR=$$(mktemp -d -t libsqlite.XXXXX)',
        'mkdir -p $$TMP_DIR',
        'cp -R $$(pwd)/external/sqlite/* $$TMP_DIR',
        'cd $$TMP_DIR',
        './configure --prefix=$$INSTALL_DIR',
        'make',
        'make install',
        'rm -rf $$TMP_DIR',
    ]),
)

cc_library(
    name = "libsqlite",
    srcs = lib_files,
    hdrs = include_files,
    includes=["include"],
    linkstatic = 1,
)
