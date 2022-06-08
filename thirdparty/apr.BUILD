licenses(["notice"])

package(default_visibility = ["//visibility:public"])

include_files = [
   "include/apr-2/apr.h",
]

lib_files = [
   "lib/libapr-2.a",
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
        'which libtool',
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
    hdrs = glob(["include/libapr-2/*.h"]),
    includes=["include/libapr-2"],
    linkstatic = 1,
)
