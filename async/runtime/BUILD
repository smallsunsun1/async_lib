package(default_visibility = ["//visibility:public"])

cc_library(
    name = "runtime",
    srcs = [
        "graph.cpp",
        "register.cpp",
        "task_graph.cpp"
    ],
    hdrs = [
        "async_kernel.h",
        "batch_task.h",
        "graph.h",
        "register.h",
        "task_graph.h"
    ],
    deps = [
        "//async/context:host_context",
        "//async/runtime/proto:cc_graph_proto",
        "@com_github_google_glog//:glog",
    ],
)