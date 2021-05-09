header_files=`find async -name "*.h" | xargs -i -P20 realpath {}`
source_files=`find async -name "*.h" | xargs -i -P20 realpath {}`

pushd build

function run_include_what_you_use() {
    python /usr/local/bin/iwyu_tool.py -p ./ -j 8 -v $1 \
    | python /usr/local/bin/fix_includes.py -b --nocomments
}

for filename in ${header_files[@]}
do 
    run_include_what_you_use ${filename}
done 

for filename in ${source_files[@]}
do 
    run_include_what_you_use ${filename}
done 

popd