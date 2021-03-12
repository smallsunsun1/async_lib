file(GLOB header_files "*.h")
file(GLOB object_files "*.cpp")
add_library(async_trace ${LIB_TYPE} ${object_files})
target_precompile_headers(async_trace PRIVATE ${header_files})