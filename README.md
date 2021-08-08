## Brief Introduction
This respority mainly borrow code from https://github.com/tensorflow/runtime.  

original LICENSE is ORI_LICENSE.

## Usage
Refer to documents and examples on tools directory, unittest and integration test are
also useful to write simple example

## Detail
Some custom execution mode such as GraphMode and TaskGraphMode are added.

Start to add some coroutine functions and tools. Currently coroutine are just under development and should not be used.

Please refer to documents for more detailed explantion.

## Feature Plan:  
1. Add unified coroutine and function task framework to process jobs.  
2. Add more utility functions for easy use this library.  

## Pre Request:  
A compiler which support c++17 feature, such as gcc9.x or newer

Bazel is the preferred build system for this repo

## Build With Cmake
(CMake build is now out of date, please use bazel build system)  
~~How to build:~~  
~~Build With Cmake~~  
~~mkdir build && cd build~~  
~~cmake -DCMAKE_BUILD_TYPE=Release ..~~  
~~cmake --build . -j\`nproc\`~~

## Build With Bazel(version 4.0.0 or newer are tested)  
### Build:  
bash scripts/build.sh
### Test:  
bash scripts/test.sh

### Clean:  
bash scripts/clean.sh
