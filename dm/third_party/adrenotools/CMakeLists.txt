

cmake_minimum_required(VERSION 3.22)
project(dm_adrenotools LANGUAGES C CXX)
set(LNB ${CMAKE_CURRENT_SOURCE_DIR}/lib/linkernsbypass)
add_library(dm_linkernsbypass STATIC ${LNB}/android_linker_ns.cpp ${LNB}/elf_soname_patcher.cpp)
target_include_directories(dm_linkernsbypass PUBLIC ${LNB})
target_link_libraries(dm_linkernsbypass PUBLIC android dl)
set_target_properties(dm_linkernsbypass PROPERTIES POSITION_INDEPENDENT_CODE ON)
add_library(dm_adrenotools STATIC src/driver.cpp)
target_include_directories(dm_adrenotools PUBLIC include PRIVATE src)
target_link_libraries(dm_adrenotools PUBLIC dm_linkernsbypass log)
set_target_properties(dm_adrenotools PROPERTIES POSITION_INDEPENDENT_CODE ON)


add_library(hook_impl SHARED src/hook/hook_impl.cpp)
target_include_directories(hook_impl PRIVATE include)
target_link_libraries(hook_impl PRIVATE dm_linkernsbypass log)
set_target_properties(hook_impl PROPERTIES CXX_VISIBILITY_PRESET hidden)
add_library(main_hook SHARED src/hook/main_hook.c)
target_link_options(main_hook PRIVATE -Wl,-z,global)
target_link_libraries(main_hook PRIVATE hook_impl)
