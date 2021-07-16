load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def repo():
    http_archive(
        name = "com_google_googletest",
        sha256 = "353571c2440176ded91c2de6d6cd88ddd41401d14692ec1f99e35d013feda55a",
        urls = ["https://github.com/google/googletest/archive/refs/tags/release-1.11.0.zip"],
        strip_prefix = "googletest-release-1.11.0",
    )
