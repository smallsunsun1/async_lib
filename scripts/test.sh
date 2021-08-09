output_base=${1:-empty}
# bazel test //... --define=malloc=tcmalloc
if [ ${output_base} != "empty" ]; then
bazel --output_base=/home/sunjiahe/bazel_build_cache/async_lib test --config=optimize --runs_per_test=1 //... --define=malloc=normal #--define=use_cxx_20=on
else
bazel test --config=optimize --runs_per_test=1 //... --define=malloc=normal #--define=use_cxx_20=on
fi 
