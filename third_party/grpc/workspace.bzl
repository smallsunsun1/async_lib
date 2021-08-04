"""load grpc third party"""
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def repo():
    http_archive(
        name = "com_github_grpc_grpc",
        sha256 = "f60e5b112913bf776a22c16a3053cc02cf55e60bf27a959fd54d7aaf8e2da6e8",
        strip_prefix = "grpc-1.38.1",
        urls = [
            "https://github.com/grpc/grpc/archive/refs/tags/v1.38.1.tar.gz",
        ],
    )