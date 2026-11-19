












cmake_minimum_required(VERSION 3.22)
project(dm_android C CXX ASM)








get_filename_component(DM_PATH_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../.. ABSOLUTE)
foreach(map "${DM_PATH_ROOT}=src" "${CMAKE_ANDROID_NDK}=ndk" "${DM_GEN_DIR}=gen" "${CMAKE_BINARY_DIR}=build")
  add_compile_options("-ffile-prefix-map=${map}")
endforeach()


add_compile_options(-fdebug-compilation-dir=.)
add_subdirectory(../third_party/spirv-tools ${CMAKE_CURRENT_BINARY_DIR}/spirv-tools)
add_subdirectory(../gfx/vulkan ${CMAKE_CURRENT_BINARY_DIR}/vulkan)



get_target_property(_eng_libs dm_vulkan_engine INTERFACE_LINK_LIBRARIES)
list(REMOVE_ITEM _eng_libs vulkan)
set_target_properties(dm_vulkan_engine PROPERTIES INTERFACE_LINK_LIBRARIES "${_eng_libs}")

set(DM ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(HOST ${DM}/host)
if(NOT DM_GEN_DIR OR NOT EXISTS ${DM_GEN_DIR}/dm_vault_tab.h)
  message(FATAL_ERROR "要 -DDM_GEN_DIR=<由游戏文件生成的宿主表头目录>（dm_vault_tab.h / dm_hud_exp.h / dm_bright_levels.inc，见 dm/host/Makefile）")
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)



set(DM_OVERLAY_TOOL ${DM}/ios/tools/overlay_contract.py)
get_filename_component(DM_REPO_ROOT ${DM}/.. ABSOLUTE)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${DM_OVERLAY_TOOL})
execute_process(COMMAND ${Python3_EXECUTABLE} ${DM_OVERLAY_TOOL} --inputs
  OUTPUT_VARIABLE overlay_inputs RESULT_VARIABLE overlay_result OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT overlay_result EQUAL 0)
  message(FATAL_ERROR "Cannot read overlay contract dependencies")
endif()
string(REPLACE "\r\n" "\n" overlay_inputs "${overlay_inputs}")
string(REPLACE "\n" ";" overlay_inputs "${overlay_inputs}")
list(TRANSFORM overlay_inputs PREPEND "${DM_REPO_ROOT}/")
set(DM_OVERLAY_HEADER ${CMAKE_CURRENT_BINARY_DIR}/dm_overlay_contract.h)
add_custom_command(OUTPUT ${DM_OVERLAY_HEADER}
  COMMAND ${Python3_EXECUTABLE} ${DM_OVERLAY_TOOL} --repo-root ${DM_REPO_ROOT} --header ${DM_OVERLAY_HEADER}
  DEPENDS ${overlay_inputs} VERBATIM)



set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${HOST}/Makefile ${CMAKE_CURRENT_SOURCE_DIR}/tools/host_sources.py)
execute_process(COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/host_sources.py
  ${HOST}/Makefile ${CMAKE_CURRENT_BINARY_DIR}/host_sources.cmake
  RESULT_VARIABLE parse_result)
if(NOT parse_result EQUAL 0)
  message(FATAL_ERROR "Cannot parse host sources for Android")
endif()
include(${CMAKE_CURRENT_BINARY_DIR}/host_sources.cmake)
file(GLOB HLE_SRCS CONFIGURE_DEPENDS ${HOST}/hle_*.c)
file(GLOB GFX_SRCS CONFIGURE_DEPENDS ${DM}/gfx/*.c ${DM}/gfx/nvn/*.c ${DM}/gfx/tex/*.c)


set(ASM_SRCS)
foreach(s ${HOST}/entry.S ${DM}/gfx/nvn/nvn_entry.S ${DM}/gfx/nvn/nvn_thunks.S)
  get_filename_component(n ${s} NAME)
  set(out ${CMAKE_CURRENT_BINARY_DIR}/asm/${n})
  add_custom_command(OUTPUT ${out} COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/asm_elf.py ${s} ${out}
                     DEPENDS ${s} ${CMAKE_CURRENT_SOURCE_DIR}/tools/asm_elf.py VERBATIM)
  list(APPEND ASM_SRCS ${out})
endforeach()


set_source_files_properties(${ASM_SRCS} PROPERTIES COMPILE_DEFINITIONS DM_NVN_COUNT_LLSC=1)


set(DM_VK_SHIM ${CMAKE_CURRENT_BINARY_DIR}/vk_shim.c)
find_path(DM_VK_HEADERS vulkan_core.h PATHS ${CMAKE_SYSROOT}/usr/include/vulkan NO_DEFAULT_PATH)
if(NOT DM_VK_HEADERS)
  message(FATAL_ERROR "vulkan_core.h not found under ${CMAKE_SYSROOT}/usr/include/vulkan")
endif()
add_custom_command(OUTPUT ${DM_VK_SHIM}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_vk_shim.py ${DM_VK_HEADERS} ${DM_VK_SHIM}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_vk_shim.py ${DM_VK_HEADERS}/vulkan_core.h ${DM_VK_HEADERS}/vulkan_android.h VERBATIM)
set_source_files_properties(${DM_VK_SHIM} PROPERTIES COMPILE_OPTIONS "-Wno-unused-parameter" INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR})

set(ANDROID_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/front_android.c
  ${CMAKE_CURRENT_SOURCE_DIR}/touch_android.c
  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan_present.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/compat/clonefile.c
  ${CMAKE_CURRENT_SOURCE_DIR}/compat/digest.c
  ${CMAKE_CURRENT_SOURCE_DIR}/compat/mach_thread.c
  ${CMAKE_CURRENT_SOURCE_DIR}/compat/mach_symbols.c
  ${DM}/gfx/vulkan/nvn_vk.c
  ${CMAKE_CURRENT_SOURCE_DIR}/vk_loader.c
  ${CMAKE_CURRENT_SOURCE_DIR}/vk_driver.c
  ${DM_VK_SHIM})



if(NOT EXISTS ${DM_GEN_DIR}/dm_touch_native_art.c)
  message(FATAL_ERROR "Generate native_art/generate.py <romfs> --output-dir <DM_GEN_DIR> first")
endif()
list(APPEND ANDROID_SRCS
  ${DM}/ios/Sources/dm_touch_game_state.c
  ${DM}/ios/Sources/dm_touch_game_identity.c
  ${DM}/ios/Sources/dm_touch_game_bridge.c
  ${DM_GEN_DIR}/dm_touch_native_art.c)

add_library(dmcore STATIC ${HOST_SRCS} ${HLE_SRCS} ${GFX_SRCS} ${LIBDEFLATE_SRCS} ${ASM_SRCS} ${ANDROID_SRCS})
list(REMOVE_DUPLICATES HOST_SRCS)
target_include_directories(dmcore PUBLIC ${HOST} ${DM}/gfx/nvn/include ${CMAKE_CURRENT_SOURCE_DIR}/compat/include ${DM_GEN_DIR})
target_include_directories(dmcore PUBLIC ${DM}/ios/Sources)


set(DM_CFLAGS -O1 -g -Wall -Wno-unused-function -fno-strict-aliasing -include ${CMAKE_CURRENT_SOURCE_DIR}/compat/include/dm_android_compat.h)
target_compile_options(dmcore PUBLIC $<$<COMPILE_LANGUAGE:C>:${DM_CFLAGS} -std=gnu17>)
target_link_libraries(dmcore PUBLIC dm_vulkan_engine aaudio android log m dl)


add_executable(dmhost ${HOST}/main.c)
target_link_libraries(dmhost PRIVATE -Wl,--whole-archive dmcore -Wl,--no-whole-archive)
target_link_options(dmhost PRIVATE -Wl,--no-undefined -Wl,-z,nostart-stop-gc -Wl,--export-dynamic)






add_library(dm_app_main OBJECT ${HOST}/main.c)
target_link_libraries(dm_app_main PRIVATE dmcore)
target_compile_definitions(dm_app_main PRIVATE main=dm_main)
add_library(dm SHARED $<TARGET_OBJECTS:dm_app_main> ${CMAKE_CURRENT_SOURCE_DIR}/jni_android.c ${CMAKE_CURRENT_SOURCE_DIR}/text_input_jni.c ${CMAKE_CURRENT_SOURCE_DIR}/touch_android_jni.c ${CMAKE_CURRENT_SOURCE_DIR}/overlay_validate.c ${DM_OVERLAY_HEADER})




set(DM_VARIANT qa CACHE STRING "Android build variant: qa or player")
if(NOT DM_VARIANT MATCHES "^(qa|player)$")
  message(FATAL_ERROR "DM_VARIANT must be qa or player, got '${DM_VARIANT}'")
endif()
if(DM_VARIANT STREQUAL qa)
  set(dm_qa_debug "$<CONFIG:Debug>")
else()
  set(dm_qa_debug 0)
endif()
target_compile_definitions(dm PRIVATE "DM_VARIANT_NAME=\"${DM_VARIANT}\"")





if(DM_VARIANT STREQUAL player)
  target_compile_definitions(dmcore PUBLIC DM_PLAYER_BUILD=1)
  target_compile_definitions(dm_vulkan_engine PRIVATE DM_PLAYER_BUILD=1)
  target_sources(dm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/player_guard.c)
  target_link_options(dm PRIVATE -Wl,--wrap=fopen)
endif()

target_sources(dm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/test_hooks.c)
target_include_directories(dm PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(dm PRIVATE $<${dm_qa_debug}:DM_ANDROID_TEST_HOOKS=1>)
target_compile_definitions(dmcore PRIVATE $<${dm_qa_debug}:DM_RECIPE_TESTS=1>)
target_compile_definitions(dmcore PRIVATE $<${dm_qa_debug}:DM_EFFECT_TESTS=1>)
target_compile_definitions(dmcore PRIVATE $<${dm_qa_debug}:DM_DRIVER_TESTS=1>)
target_link_libraries(dm PRIVATE -Wl,--whole-archive dmcore -Wl,--no-whole-archive android)
target_link_options(dm PRIVATE -Wl,--no-undefined -Wl,-z,nostart-stop-gc -Wl,-Bsymbolic)






add_subdirectory(../third_party/adrenotools ${CMAKE_CURRENT_BINARY_DIR}/adrenotools)
add_library(dm_driverload SHARED ${CMAKE_CURRENT_SOURCE_DIR}/driverload.c)
target_link_libraries(dm_driverload PRIVATE dm_adrenotools log)
target_link_options(dm_driverload PRIVATE -Wl,--no-undefined -Wl,--exclude-libs,ALL)
set_target_properties(dm_driverload PROPERTIES C_VISIBILITY_PRESET hidden)
