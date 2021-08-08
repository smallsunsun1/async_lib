# bazel build //... --define=malloc=tcmalloc
bazel build //... --define=malloc=normal #--define=use_cxx_20=on
