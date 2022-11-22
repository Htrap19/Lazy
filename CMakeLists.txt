cmake_minimum_required(VERSION 3.23)
project(sandbox)

set(CMAKE_CXX_STANDARD 20)
add_subdirectory(lazy)

add_executable(sandbox sandbox/src/sandbox.cpp)
target_include_directories(sandbox PUBLIC lazy/src)