cmake_minimum_required(VERSION 3.0)
project(nonlocality)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -rdynamic")

add_subdirectory(src)
