# bazel build //... --define=malloc=tcmalloc
bazel build --config=optimize //... --define=malloc=normal --define=use_cxx_20=on
