if (POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)
project(project_async)
cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED 14)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set(LIB_TYPE "STATIC" CACHE STRING "build library type")
set(CMAKE_PREFIX_PATH libs/)

add_definitions(-DEIGEN_USE_THREADS)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-offsetof")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -flto")

option(USE_ASAN "variable which indicate whether to use address sanitizer" OFF)
option(USE_CUDA "variable which indicate whether to build with cuda" OFF)
if(USE_ASAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -O1 -fno-omit-frame-pointer")
endif()
if(USE_CUDA)
    enable_language(CUDA)
    add_definitions(-DEIGEN_USE_GPU)
endif()


include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(
    googletest 
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.10.0
)
FetchContent_MakeAvailable(googletest)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.9.0
)
FetchContent_MakeAvailable(nlohmann_json)
# FetchContent_Declare(
#     gRPC 
#     GIT_REPOSITORY https://github.com/grpc/grpc
#     GIT_TAG v1.35.0
# )
# FetchContent_MakeAvailable(gRPC)
# FetchContent_Declare(
#     absl 
#     GIT_REPOSITORY https://github.com/abseil/abseil-cpp
#     GIT_TAG lts_2020_09_23
# )
# FetchContent_MakeAvailable(absl)

enable_testing()

include_directories(third_party/libs/include)
include_directories(third_party/eigen3)
include_directories(third_party/abseil-cpp)
include_directories(./)
# link_directories(third_party/libs/lib)
link_libraries(pthread)
add_subdirectory(third_party)
add_subdirectory(async)
add_subdirectory(tools)

# find_package(OpenCV REQUIRED)

# add_executable(main main.cpp)
# target_include_directories(main PRIVATE ${OpenCV_INCLUDE_DIRS})
# target_link_libraries(main PRIVATE ${OpenCV_LIBRARIES})