package(default_visibility = ["//visibility:public"])

cc_library(
    name = "concurrent",
    srcs = [
        "concurrent_work_queue.cpp",
    ],
    hdrs = [
        "concurrent_work_queue.h",
        "environment.h",
        "event_count.h",
        "non_blocking_work_queue.h",
        "blocking_work_queue.h",
        "task_deque.h",
        "task_priority_queue.h",
        "task_queue.h",
        "work_queue_base.h",
    ],
    deps = [
        "//async/support:support",
    ],
)