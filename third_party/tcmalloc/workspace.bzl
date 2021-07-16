load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def repo():
    http_archive(
        name = "com_google_tcmalloc",
        urls = ["https://github.com/google/tcmalloc/archive/refs/heads/master.zip"],
        strip_prefix = "tcmalloc-master",
    )   