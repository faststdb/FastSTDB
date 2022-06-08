licenses(["notice"])

package(default_visibility = ["//visibility:public"])

include_files = [
   "include/expat_config.h",
   "include/expat_external.h",
   "include/expat.h",
]

lib_files = [
   "lib/libexpat.a",
]

genrule(
    name = "libexpat-srcs",
    outs = include_files + lib_files,
    cmd = "\n".join([
        'set -x',
        'export INSTALL_DIR=$$(pwd)/$(@D)',
        'export TMP_DIR=$$(mktemp -d -t libexpat.XXXXX)',
        'mkdir -p $$TMP_DIR',
        'cp -R $$(pwd)/external/expat/* $$TMP_DIR',
        'cd $$TMP_DIR',
        'cd expat',
        'sh buildconf.sh',
        './configure --prefix=$$INSTALL_DIR',
        'make',
        'make install',
        'rm -rf $$TMP_DIR',
    ]),
)

cc_library(
    name = "libexpat",
    srcs = lib_files,
    hdrs = include_files,
    includes=["include"],
    linkstatic = 1,
)
