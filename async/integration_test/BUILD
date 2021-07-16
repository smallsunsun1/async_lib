package(default_visibility = ["//visibility:public"])

config_setting(
    name = "custom_malloc",
    define_values = {"malloc" : "tcmalloc"},
)

malloc = select({
    ":custom_malloc": "@com_google_tcmalloc//tcmalloc",
    "//conditions:default": "@bazel_tools//tools/cpp:malloc"
})

cc_test(
    name = "test_async_graph",
    deps = [
        "//async/runtime:runtime",
    ],
    malloc = "@com_google_tcmalloc//tcmalloc",
    srcs = [
        "end2end_test_async_graph.cpp",
    ]
)

cc_test(
    name = "test_task_graph",
    deps = [
        "//async/runtime:runtime",
    ],
    srcs = [
        "end2end_test_task_graph.cpp",
    ],
    malloc = malloc,
)

cc_test(
    name = "openmp_func_test",
    srcs = [
        "openmp_function_test.cpp",
    ],
    size = "small",
    copts = ["-fopenmp"],
    linkopts = ["-fopenmp"],
    malloc = malloc,
)

cc_test(
    name = "openmp_perf_test",
    deps = [
        "//async/runtime:runtime",
        "@com_google_googletest//:gtest"
    ],
    size = "small",
    srcs = [
        "openmp_perf_compare.cpp",
    ],
    copts = ["-fopenmp"],
    linkopts = ["-fopenmp"],
    malloc = malloc,
)