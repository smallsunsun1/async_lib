"""load gtest third party"""
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def repo():
    http_archive(
        name = "rules_fuzzing",
        sha256 = "127d7c45e9b7520b3c42145b3cb1b8c26477cdaed0521b02a0298907339fefa1",
        strip_prefix = "rules_fuzzing-0.2.0",
        urls = ["https://github.com/bazelbuild/rules_fuzzing/archive/v0.2.0.zip"],
    )
