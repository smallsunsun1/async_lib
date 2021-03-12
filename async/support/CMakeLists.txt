file(GLOB header_files "*.h")
file(GLOB object_files "*.cpp")
add_library(async_support ${LIB_TYPE} ${object_files})
target_precompile_headers(async_support PRIVATE ${header_files})