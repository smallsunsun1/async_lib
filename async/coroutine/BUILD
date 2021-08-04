package(default_visibility = ["//visibility:public"])

config_setting(
    name = "with_coroutine",
    define_values = {"use_cxx_20" : "on"},
)

ori_header_files = glob(["*.h", "internal/*.h"])
ori_src_files = glob(["*.cpp"])

header_files = select({
    ":with_coroutine": ori_header_files,
    "//conditions:default": [] 
})

src_files = select({
    ":with_coroutine": ori_src_files,
    "//conditions:default": []
})

cc_library(
    name = "coroutine",
    hdrs = header_files,
    srcs = src_files,
    copts = [
        "-fcoroutines",
        "-std=c++20",
    ],
    deps = [
        "//async/concurrent:concurrent",
        "@com_google_absl//absl/synchronization:synchronization",
    ]
)

cc_test(
    name = "coroutine_test",
    srcs = select({
        ":with_coroutine": ["tests/coroutine_test.cpp"],
        "//conditions:default": ["tests/empty_test.cpp"]
    }),
    deps = [
        ":coroutine",
    ],
    copts = select({
        ":with_coroutine": ["-fcoroutines", "-std=c++20"],
        "//conditions:default": []
    }),
)

cc_test(
    name = "coroutine_thread_pool_test",
    srcs = select({
        ":with_coroutine": ["tests/coroutine_thread_pool_test.cpp"],
        "//conditions:default": ["tests/empty_test.cpp"]
    }),
    deps = [
        ":coroutine",
    ],
    copts = select({
        ":with_coroutine": ["-fcoroutines", "-std=c++20"],
        "//conditions:default": []
    }),
)

cc_test(
    name = "when_all_test",
    srcs = select({
        ":with_coroutine": ["tests/when_all_test.cpp"],
        "//conditions:default": ["tests/empty_test.cpp"]
    }),
    deps = [
        ":coroutine",
    ],
    copts = select({
        ":with_coroutine": ["-fcoroutines", "-std=c++20"],
        "//conditions:default": []
    }),
)
