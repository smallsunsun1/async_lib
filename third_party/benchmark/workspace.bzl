load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def repo():
    http_archive(
        name = "com_github_google_benchmark",
        urls = ["https://github.com/google/benchmark/archive/refs/tags/v1.5.5.zip"],
        strip_prefix = "benchmark-1.5.5",
    )