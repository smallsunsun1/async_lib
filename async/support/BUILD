package(default_visibility = ["//visibility:public"])

cc_library(
    name = "support",
    srcs = [
        "alloc.cpp",
        "ref_count.cpp",
        "string_util.cpp",
    ],
    hdrs = [
        "type_traits.h",
        "thread_local.h",
        "latch.h",
        "unique_function.h",
        "ref_count.h",
        "alloc.h",
        "extra_structure.h",
        "string_util.h",
        "rc_array.h",
        "function_ref.h",
        "concurrent_vector.h",
        "task_function.h"
    ],
    deps = [
        "@absl//absl/types:span"
    ],
)