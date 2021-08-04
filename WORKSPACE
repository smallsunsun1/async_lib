workspace(name = "async")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "com_grail_bazel_compdb",
    sha256 = "df63430df795293fe49d2b71a731278038da1d95f00e3c2b9efd141126c1f3d9",
    strip_prefix = "bazel-compilation-database-master",
    urls = ["https://github.com/grailbio/bazel-compilation-database/archive/master.tar.gz"],
)

http_archive(
    name = "bazel_skylib",
    sha256 = "1c531376ac7e5a180e0237938a2536de0c54d93f5c278634818e0efc952dd56c",
    urls = [
        "https://github.com/bazelbuild/bazel-skylib/releases/download/1.0.3/bazel-skylib-1.0.3.tar.gz",
        "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.0.3/bazel-skylib-1.0.3.tar.gz",
    ],
)

load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")

bazel_skylib_workspace()

http_archive(
  name = "rules_cc",
  sha256 = "9a446e9dd9c1bb180c86977a8dc1e9e659550ae732ae58bd2e8fd51e15b2c91d",
  urls = ["https://github.com/bazelbuild/rules_cc/archive/262ebec3c2296296526740db4aefce68c80de7fa.zip"],
  strip_prefix = "rules_cc-262ebec3c2296296526740db4aefce68c80de7fa",
)

http_archive(
    name = "rules_fuzzing",
    sha256 = "127d7c45e9b7520b3c42145b3cb1b8c26477cdaed0521b02a0298907339fefa1",
    strip_prefix = "rules_fuzzing-0.2.0",
    urls = ["https://github.com/bazelbuild/rules_fuzzing/archive/v0.2.0.zip"],
)

load("@rules_fuzzing//fuzzing:repositories.bzl", "rules_fuzzing_dependencies")
rules_fuzzing_dependencies()

load("@rules_fuzzing//fuzzing:init.bzl", "rules_fuzzing_init")
rules_fuzzing_init()

load("//third_party/gtest:workspace.bzl", gtest = "repo")
gtest()

load("//third_party/absl:workspace.bzl", absl = "repo")
absl()

load("//third_party/glog:workspace.bzl", glog = "repo")
glog()

load("//third_party/gflags:workspace.bzl", gflags = "repo")
gflags()

load("//third_party/tcmalloc:workspace.bzl", tcmalloc = "repo")
tcmalloc()

load("//third_party/benchmark:workspace.bzl", benchmark = "repo")
benchmark()

load("//third_party/protobuf:workspace.bzl", protobuf = "repo")
protobuf()

http_archive(
    name = "rules_proto",
    sha256 = "602e7161d9195e50246177e7c55b2f39950a9cf7366f74ed5f22fd45750cd208",
    strip_prefix = "rules_proto-97d8af4dc474595af3900dd85cb3a29ad28cc313",
    urls = [
        "https://github.com/bazelbuild/rules_proto/archive/97d8af4dc474595af3900dd85cb3a29ad28cc313.tar.gz",
    ],
)

load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies")
rules_proto_dependencies()

http_archive(
    name = "com_github_grpc_grpc",
    sha256 = "f60e5b112913bf776a22c16a3053cc02cf55e60bf27a959fd54d7aaf8e2da6e8",
    strip_prefix = "grpc-1.38.1",
    urls = [
        "https://github.com/grpc/grpc/archive/refs/tags/v1.38.1.tar.gz",
    ],
)
load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")
grpc_deps()
load("@com_github_grpc_grpc//bazel:grpc_extra_deps.bzl", "grpc_extra_deps")
grpc_extra_deps()