output_base=${1:-empty}
echo ${output_base}
# bazel build //... --define=malloc=tcmalloc
if [ ${output_base} != "empty" ]; then
bazel --output_base=/home/sunjiahe/bazel_build_cache/async_lib build --config=optimize //... --define=malloc=normal --define=use_cxx_20=on
else
bazel build --config=optimize //... --define=malloc=normal --define=use_cxx_20=on
fi

