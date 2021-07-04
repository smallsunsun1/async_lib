cmake_minimum_required(VERSION 3.15)
project(project_async VERSION 1.0.0)
set(CMAKE_CXX_STANDARD_REQUIRED 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set(LIB_TYPE "STATIC" CACHE STRING "build library type")
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};third_party_libs")

add_definitions(-DEIGEN_USE_THREADS)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-offsetof -fdiagnostics-color=always -march=native -flto")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -Wall -Wextra -Wno-return-type -Wno-maybe-uninitialized")

## Check Whether Compiler Is Gcc
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_AR "gcc-ar")
    set(CMAKE_RANLIB "gcc-ranlib")
endif()

option(USE_ASAN "variable which indicate whether to use address sanitizer" OFF)
option(USE_CUDA "variable which indicate whether to build with cuda" OFF)
option(USE_CXX_20 "variable which indicate whether to build with -std=c++20 standard" OFF)
if(USE_ASAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -O1 -fno-omit-frame-pointer")
endif()
if(USE_CUDA)
    set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)
    set(CUDA_INCLUDES "/usr/local/cuda/include/")
    enable_language(CUDA)
    add_definitions(-DEIGEN_USE_GPU)
    include_directories(${CUDA_INCLUDES})
endif()
if(USE_CXX_20)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -fcoroutines")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(
    googletest 
    URL    https://github.com/google/googletest/archive/refs/tags/release-1.11.0.tar.gz
)
FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)
  FetchContent_Populate(googletest)
  add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
endif()

FetchContent_Declare(
    abseil 
    URL    https://github.com/abseil/abseil-cpp/archive/refs/tags/20210324.2.tar.gz
)
FetchContent_GetProperties(abseil)
if(NOT abseil_POPULATED)
  FetchContent_Populate(abseil)
  add_subdirectory(${abseil_SOURCE_DIR} ${abseil_BINARY_DIR})
endif()

enable_testing()

include_directories(${CMAKE_SOURCE_DIR})

find_package(Threads REQUIRED)
add_subdirectory(async)
add_subdirectory(tools)

if (USE_CXX_20)
  set(coroutine_lib async_coroutine)
else()
  set(coroutine_lib "")
endif()
install(TARGETS async_context async_runtime async_concurrent async_support async_trace ${coroutine_lib}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/async_lib)

