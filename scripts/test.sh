# bazel test //... --define=malloc=tcmalloc
bazel test --runs_per_test=10 //... --define=malloc=normal --define=use_cxx_20=on