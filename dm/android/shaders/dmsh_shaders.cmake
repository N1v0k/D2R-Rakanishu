

find_package(Threads REQUIRED)
add_library(dmsh_shaders SHARED ${CMAKE_CURRENT_LIST_DIR}/dmsh_shaders.cpp)
target_link_libraries(dmsh_shaders PRIVATE dm_spirv_opt Threads::Threads ${CMAKE_DL_LIBS})


target_compile_options(dmsh_shaders PRIVATE -O2 -g0 -fvisibility=hidden -Wall -Wextra)
target_link_options(dmsh_shaders PRIVATE -Wl,--no-undefined -Wl,--gc-sections -Wl,--exclude-libs,ALL -Wl,--strip-debug)
if(ANDROID)
  target_link_options(dmsh_shaders PRIVATE -Wl,-z,max-page-size=16384)
endif()
