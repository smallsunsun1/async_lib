load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def repo():
    http_archive(
        name = "absl",
        urls = ["https://github.com/abseil/abseil-cpp/archive/refs/tags/20210324.2.zip"],
        sha256 = "1a7edda1ff56967e33bc938a4f0a68bb9efc6ba73d62bb4a5f5662463698056c",
        strip_prefix = "abseil-cpp-20210324.2",
    )
