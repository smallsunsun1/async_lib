load("@com_grail_bazel_compdb//:aspects.bzl", "compilation_database")

compilation_database(
    name = "async_database",
    targets = [
        "//async/concurrent:concurrent",
        "//async/context:host_context",
        "//async/runtime:runtime",
        "//async/support:support",
    ]
)