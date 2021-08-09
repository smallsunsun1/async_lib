# bazel test //... --define=malloc=tcmalloc
bazel test --config=optimize --runs_per_test=5 //... --define=malloc=normal #--define=use_cxx_20=on
