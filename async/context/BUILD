package(default_visibility = ["//visibility:public"])

cc_library(
    name = "host_context",
    srcs = [
        "host_context.cpp",
        "async_value_ref.cpp",
        "async_value.cpp",
        "diagnostic.cpp",
        "function.cpp",
        "host_allocator.cpp",
        "host_buffer.cpp",
        "host_context_ptr.cpp",
        "native_function.cpp",
        "profiled_allocator.cpp",
        "single_threaded_work_queue.cpp",
        "multi_threaded_work_queue.cpp",
    ],
    hdrs = [
        "host_context.h",
        "async_value_ref.h",
        "async_value.h",
        "diagnostic.h",
        "function.h",
        "execution_context.h",
        "host_allocator.h",
        "host_buffer.h",
        "host_context_ptr.h",
        "native_function.h",
        "location.h",
        "kernel_frame.h",
        "resource.h",
        "chain.h",
    ],
    deps = [
        "//async/support:support",
        "//async/concurrent:concurrent",
        "@absl//absl/strings:strings"
    ],
)

