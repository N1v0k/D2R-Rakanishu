



cmake_minimum_required(VERSION 3.22)
project(dm_spirv_tools LANGUAGES CXX)
set(S ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB DM_SPIRV_TOOLS_SRCS ${S}/source/*.cpp ${S}/source/util/*.cpp ${S}/source/val/*.cpp ${S}/source/opt/*.cpp)
add_library(dm_spirv_opt STATIC ${DM_SPIRV_TOOLS_SRCS})
target_include_directories(dm_spirv_opt PUBLIC ${S}/include PRIVATE ${S} ${S}/generated ${S}/spirv-headers/include)
target_compile_features(dm_spirv_opt PUBLIC cxx_std_17)



target_compile_options(dm_spirv_opt PRIVATE -O2 -g0 -fno-rtti -w)
set_target_properties(dm_spirv_opt PROPERTIES POSITION_INDEPENDENT_CODE ON)
