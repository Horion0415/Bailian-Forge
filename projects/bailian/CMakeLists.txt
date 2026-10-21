cmake_minimum_required(VERSION 3.22)
set(MOSAICO_UTILS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../submodule/esp-mosaico-utils")
set(MOSAICO_BSP_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../submodule/esp-mosaico-bsp")
set(RAYLIB_LITE_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../submodule/raylib-lite-engine")
if(NOT SDKCONFIG)
    set(SDKCONFIG "${CMAKE_BINARY_DIR}/sdkconfig")
endif()
if(NOT SDKCONFIG_DEFAULTS)
    set(SDKCONFIG_DEFAULTS "sdkconfig.defaults;sdkconfig.application.defaults")
endif()
include("${MOSAICO_UTILS_ROOT}/mosaico-tools/cmake/raylib_lite_engine.cmake")
mosaico_game_sdk_configure_gsp_compiler()
mosaico_game_sdk_add_components(RAYLIB AUDIO)
include("${MOSAICO_UTILS_ROOT}/esp-mosaico-recovery/cmake/mosaico_idf_project.cmake")
project(bailian VERSION 0.3.3)
# The pinned S31 GCC 16.1 crashes in SLP across both game and SDK sources
# (lwIP and DMA included). Keep other -O2 passes and other toolchains intact.
if(IDF_TARGET STREQUAL "esp32s31" AND CMAKE_C_COMPILER_ID STREQUAL "GNU" AND
   CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL "16.1" AND
   CMAKE_C_COMPILER_VERSION VERSION_LESS "16.2")
    idf_build_set_property(COMPILE_OPTIONS "-fno-tree-slp-vectorize" APPEND)
endif()
include("${MOSAICO_UTILS_ROOT}/mosaico-tools/cmake/system_update.cmake")
