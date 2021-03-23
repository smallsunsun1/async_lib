使用run_clang_format.sh脚本来保证代码格式的一致性  
使用run_clang_format.sh需要先安装clang-format程序，通常可以用以下两种方式安装:  
1. sudo apt-get install clang-format
2. git clone https://github.com/llvm/llvm-project.git, 在编译llvm的时候添加对clang子项目的编译即可，编译完可以得到clang-format工具