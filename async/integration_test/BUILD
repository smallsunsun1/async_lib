package(default_visibility = ["//visibility:public"])

cc_test(
    name = "test_async_graph",
    deps = [
        "//async/runtime:runtime",
    ],
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
    ]
)

cc_test(
    name = "openmp_func_test",
    srcs = [
        "openmp_function_test.cpp",
    ],
    size = "small",
    copts = ["-fopenmp"],
    linkopts = ["-fopenmp"]
)

cc_test(
    name = "openmp_perf_test",
    deps = [
        "//async/runtime:runtime",
        "@googletest//:gtest"
    ],
    size = "small",
    srcs = [
        "openmp_perf_compare.cpp",
    ],
    copts = ["-fopenmp"],
    linkopts = ["-fopenmp"]
)