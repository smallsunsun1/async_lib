## Brief Introduction
This respority mainly borrow code from https://github.com/tensorflow/runtime.  

original LICENSE is ORI_LICENSE.

For easy of use, this repo only depend on abseil for thirdparty dependency(only depend on absl::Span), can be removed easily.  
googletest, benchmark just for test use.  
tcmalloc is for performance consideration in some situation. 

Some custom execution mode such as GraphMode and TaskGraphMode are added.

Start to add some coroutine functions and tools. Currently coroutine are just under development and should not be used.

Please refer to documents for more detailed explantion.

## Feature Plan:  
1. Add unified coroutine and function task framework to process jobs.  
2. Add more utility functions for easy use this library.  

## Pre Request:  
A compiler which support c++17 feature, such as gcc9.x or newer

Bazel is the preferred build system for this repo

How to build:  
Build With Cmake  
mkdir build && cd build  
cmake -DCMAKE_BUILD_TYPE=Release ..  
cmake --build . -j\`nproc\`

Build With Bazel(version 4.0.0 or upper are tested)  
bash scripts/build.sh