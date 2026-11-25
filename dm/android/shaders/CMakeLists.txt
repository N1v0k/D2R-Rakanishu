
cmake_minimum_required(VERSION 3.22)
project(dmsh_shaders_host CXX)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/spirv-tools ${CMAKE_CURRENT_BINARY_DIR}/spirv-tools)
include(${CMAKE_CURRENT_SOURCE_DIR}/dmsh_shaders.cmake)
