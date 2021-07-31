load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def repo():
    http_archive(
        name = "com_github_google_benchmark",
        sha256 = "30f2e5156de241789d772dd8b130c1cb5d33473cc2f29e4008eab680df7bd1f0",
        urls = ["https://github.com/google/benchmark/archive/refs/tags/v1.5.5.zip"],
        strip_prefix = "benchmark-1.5.5",
    )