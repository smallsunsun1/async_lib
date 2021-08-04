"""load abseil third party"""
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def repo():
    http_archive(
        name = "com_google_absl",  # 2021-05-20T02:59:16Z
        urls = ["https://github.com/abseil/abseil-cpp/archive/7971fb358ae376e016d2d4fc9327aad95659b25e.zip"],
        strip_prefix = "abseil-cpp-7971fb358ae376e016d2d4fc9327aad95659b25e",
        sha256 = "aeba534f7307e36fe084b452299e49b97420667a8d28102cf9a0daeed340b859",
    )
