



cmake_minimum_required(VERSION 3.22)
project(dm_entry C CXX ASM)
include(${CMAKE_CURRENT_SOURCE_DIR}/local.cmake OPTIONAL)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../../../native ${CMAKE_CURRENT_BINARY_DIR}/native)
