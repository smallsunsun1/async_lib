workspace(name = "async")

load("//third_party/bazel_skylib:workspace.bzl", bazel_skylib = "repo")
bazel_skylib()
load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")
bazel_skylib_workspace()

load("//third_party/rules_fuzzing:workspace.bzl", rules_fuzzing = "repo")
rules_fuzzing()
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

load("//third_party/grpc:workspace.bzl", grpc = "repo")
grpc()
load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")
grpc_deps()
load("@com_github_grpc_grpc//bazel:grpc_extra_deps.bzl", "grpc_extra_deps")
grpc_extra_deps()
