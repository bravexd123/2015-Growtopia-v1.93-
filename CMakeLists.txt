cmake_minimum_required(VERSION 3.22)
project(GrowtopiaServer)

# Include the inner directory containing the headers
include_directories(libraries/magic_enum/include/magic_enum)

add_subdirectory(src)
add_subdirectory(libraries)