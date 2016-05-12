cmake_minimum_required(VERSION 3.5)
project(dynamic_allocator)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.c)
add_executable(dynamic_allocator ${SOURCE_FILES})