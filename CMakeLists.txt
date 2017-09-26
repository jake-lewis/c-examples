cmake_minimum_required(VERSION 3.8)
project(c_examples)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES main.c debug_util.c debug_util.h)
add_executable(c_examples ${SOURCE_FILES})