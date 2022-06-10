licenses(["notice"])

package(default_visibility = ["//visibility:public"])

include_files = [
"include/muParserBase.h",
"include/muParserBytecode.h",
"include/muParserCallback.h",
"include/muParserDef.h",
"include/muParserDLL.h",
"include/muParserError.h",
"include/muParserFixes.h",
"include/muParser.h",
"include/muParserInt.h",
"include/muParserTemplateMagic.h",
"include/muParserTest.h",
"include/muParserToken.h",
"include/muParserTokenReader.h",
]

lib_files = [
    "lib/libmuparser.a",
]

genrule(
    name = "libmuparser-srcs",
    outs = include_files + lib_files,
    cmd = "\n".join([
        'set -x',
        'export INSTALL_DIR=$$(pwd)/$(@D)',
        'export TMP_DIR=$$(mktemp -d -t libmuparser.XXXXX)',
        'mkdir -p $$TMP_DIR',
        'cp -R $$(pwd)/external/muparser/* $$TMP_DIR',
        'cd $$TMP_DIR',
        'mkdir build',
        'cd build',
        'cmake ../  -DCMAKE_INSTALL_PREFIX=$$INSTALL_DIR -DCMAKE_INSTALL_LIBDIR=$$INSTALL_DIR/lib -DBUILD_SHARED_LIBS=OFF -DENABLE_OPENMP=OFF',
        'make',
        'make install',
        'rm -rf $$TMP_DIR',
    ]),
)

cc_library(
    name = "libmuparser",
    srcs = lib_files,
    hdrs = include_files,
    includes=["include"],
    linkstatic = 1,
)
