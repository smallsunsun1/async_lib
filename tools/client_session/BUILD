package(default_visibility = ["//visibility:public"])

cc_library(
    name = "sample_client",
    srcs = [
        "sample_client.cpp",
    ],
    hdrs = [
        "sample_client.h",
    ],
    deps = [
        "//async/runtime",
        "//tools/proto:grpc_message_cc_proto",
        "@com_github_google_glog//:glog",
        "@com_github_grpc_grpc//:grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
    ],
)

cc_binary(
    name = "client_session",
    srcs = [
        "client_session.cpp",
    ],
    deps = [
        ":sample_client",
    ],
)
