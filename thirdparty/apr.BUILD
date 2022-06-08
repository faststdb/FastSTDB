licenses(["notice"])

package(default_visibility = ["//visibility:public"])

include_files = glob([
   "include/libapr-1/*.h",
])

lib_files = [
   "lib/libapr-1.a",
]

genrule(
    name = "libapr-srcs",
    outs = include_files + lib_files,
    srcs = [
       "@expat//:libexpat",     
       "@sqlite//:libsqlite",
    ],
    cmd = "\n".join([
        'set -x',
        'export INSTALL_DIR=$$(pwd)/$(@D)',
        'export TMP_DIR=$$(mktemp -d -t libapr.XXXXX)',
        'mkdir -p $$TMP_DIR',
        'cp -R $$(pwd)/external/apr/* $$TMP_DIR',
        'cd $$TMP_DIR',
        './buildconf',
        './configure --prefix=$$INSTALL_DIR --with-expat=$$INSTALL_DIR/../expat/ --with-sqlite3=$$INSTALL_DIR/../sqlite --enable-shared=false',
        'make',
        'make install',
        'rm -rf $$TMP_DIR',
    ]),
)

cc_library(
    name = "libapr",
    srcs = lib_files,
    hdrs = include_files,
    includes=["include/libapr-1"],
    linkstatic = 1,
)
