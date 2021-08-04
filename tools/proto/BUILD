load("@com_github_grpc_grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")

package(default_visibility = ["//visibility:public"])

cc_grpc_library(
    name = "grpc_message_cc_proto",
    srcs = [
        "message.proto",
    ],
    deps = [],
)

proto_library(
    name = "config_proto",
    srcs = [
        "config.proto",
    ]
)

cc_proto_library(
    name = "cc_config_proto",
    deps = [
        "config_proto",
    ]
)
