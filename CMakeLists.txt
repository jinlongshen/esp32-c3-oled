cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(esp32c3_demo)

# Export these variables to the CMake cache so components can see them
set(FREETYPE_LIB "/opt/freetype/lib/libfreetype.a" CACHE STRING "Freetype library")
set(FREETYPE_INCLUDE "/opt/freetype/include/freetype2" CACHE STRING "Freetype include path")

set(LVGL_INCLUDE "/opt/lvgl/include" CACHE STRING "LVGL include path")
set(LVGL_LIB_DIR "/opt/lvgl/lib" CACHE STRING "LVGL library directory")
set(LVGL_CONF_PATH "/opt/lvgl/include/lvgl/lv_conf.h" CACHE STRING "LVGL config header")