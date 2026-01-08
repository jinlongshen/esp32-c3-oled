cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32c3_demo)

set(FREETYPE_LIB "/opt/freetype/lib/libfreetype.a" CACHE STRING "Freetype library")
set(FREETYPE_INCLUDE "/opt/freetype/include/freetype2" CACHE STRING "Freetype include path")

set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/components/lvgl_conf/lv_conf.h")


