cmake_minimum_required(VERSION 3.8)
project(untitled)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES main.c)
add_executable(untitled ${SOURCE_FILES})

set(Sources my_lib.cpp)
set(Headers header_of_my_lib.h)