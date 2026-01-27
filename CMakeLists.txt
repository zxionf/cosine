cmake_minimum_required(VERSION 3.15)
project(cosvideo)

set(CMAKE_CXX_STANDARD 20)
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# file(GLOB_RECURSE SOURES "")

add_subdirectory(test)

add_subdirectory(movy)
add_subdirectory(interpreter)