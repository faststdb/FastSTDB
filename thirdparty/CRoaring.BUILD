licenses(["notice"])

package(default_visibility = ["//visibility:public"])

include_files = [
"include/roaring/roaring64map.hh",
"include/roaring/roaring.hh",
"include/roaring/array_util.h",
"include/roaring/bitset_util.h",
"include/roaring/isadetection.h",
"include/roaring/memory.h",
"include/roaring/portability.h",
"include/roaring/roaring_array.h",
"include/roaring/roaring.h",
"include/roaring/roaring_types.h",
"include/roaring/roaring_version.h",
"include/roaring/utilasm.h",
"include/roaring/containers/array.h",
"include/roaring/containers/bitset.h",
"include/roaring/containers/container_defs.h",
"include/roaring/containers/containers.h",
"include/roaring/containers/convert.h",
"include/roaring/containers/mixed_andnot.h",
"include/roaring/containers/mixed_equal.h",
"include/roaring/containers/mixed_intersection.h",
"include/roaring/containers/mixed_negation.h",
"include/roaring/containers/mixed_subset.h",
"include/roaring/containers/mixed_union.h",
"include/roaring/containers/mixed_xor.h",
"include/roaring/containers/perfparameters.h",
"include/roaring/containers/run.h",
"include/roaring/misc/configreport.h",
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
        'export TMP_DIR=$$(mktemp -d -t libcroarings.XXXXX)',
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
