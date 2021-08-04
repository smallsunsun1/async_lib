package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "sample_worker",
    srcs = [
        "sample_worker.cpp",
    ],
    deps = [
        "//async/runtime",
        "//tools/proto:grpc_message_cc_proto",
        "@com_github_grpc_grpc//:grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
    ],
)
