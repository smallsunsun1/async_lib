package(default_visibility = ["//visibility:public"])


py_library(
    name = "adhoc_graph_proto",
    srcs = [
        "node_pb2.py",
        "graph_pb2.py",
    ],
)

py_binary(
    name = "visualize_graph",
    args = [
        "--filename ./tools/data/graph2.pb.txt",
    ],
    srcs = [
        "visualize_graph.py",
    ],
    deps = [
        "//async/runtime/proto:py_graph_proto",
        ":adhoc_graph_proto",
    ],
    data = [
        "//tools/data:graph_def",
    ]
)
