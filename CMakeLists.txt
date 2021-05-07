if (POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)
project(project_async)
cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD_REQUIRED 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set(LIB_TYPE "STATIC" CACHE STRING "build library type")
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};third_party_libs")

add_definitions(-DEIGEN_USE_THREADS)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-offsetof")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")

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
    set(CMAKE_CXX_FLAGS "-std=c++1z")
endif()


include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(
    googletest 
    URL    https://github.com/google/googletest/archive/release-1.10.0.tar.gz
)
FetchContent_MakeAvailable(googletest)

# FetchContent_Declare(
#     gRPC 
#     GIT_REPOSITORY https://github.com/grpc/grpc
#     GIT_TAG        v1.35.0
# )
# FetchContent_MakeAvailable(gRPC)

enable_testing()

# find_package(OpenCV REQUIRED)

include_directories(third_party/eigen3)
include_directories(./)
include_directories(third_party/abseil-cpp)

link_libraries(pthread)
add_subdirectory(third_party)
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



