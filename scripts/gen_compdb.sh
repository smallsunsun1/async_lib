bazel build //:async_database
outfile="$(bazel info bazel-bin)/compile_commands.json"
execroot=$(bazel info execution_root)
sed -i.bak "s@__EXEC_ROOT__@${execroot}@" "${outfile}"
sed -i "s#-fno-canonical-system-headers##g;s#-std=c++0x ##g" ${outfile}