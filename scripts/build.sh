# bazel build //... --define=malloc=tcmalloc
bazel --output_base=/home/sunjiahe/bazel_build_cache/async_lib build --config=optimize //... --define=malloc=normal --define=use_cxx_20=on
