load("@com_google_protobuf//:protobuf.bzl", "py_proto_library")
package(default_visibility = ["//visibility:public"])

proto_library(
    name = "node_proto",
    srcs = [
        "node.proto",
    ]
)

proto_library(
    name = "graph_proto",
    srcs = [
        "graph.proto",
    ],
    deps = [
        "node_proto",
    ]
)

cc_proto_library(
    name = "cc_node_proto",
    deps = [
        ":node_proto",
    ]
)

py_proto_library(
    name = "py_node_proto",
    srcs = [
        "node.proto",
    ]
)

cc_proto_library(
    name = "cc_graph_proto",
    deps = [
        ":graph_proto",
    ]
)

py_proto_library(
    name = "py_graph_proto",
    srcs = [
        "graph.proto",
    ],
    deps = [
        ":py_node_proto",
    ]
)
