set(LVGL_ROOT "/opt/lvgl")
set(FREETYPE_ROOT "/opt/freetype")

set(LV_LIB "${LVGL_ROOT}/lib/liblvgl.a")
set(FT_LIB "${FREETYPE_ROOT}/lib/libfreetype.a")

idf_component_register(
    SRCS "src/lvgl_driver.cpp"
    INCLUDE_DIRS "inc"
    REQUIRES oled
)

# This is the "Root Cause" fix for the FreeType error
target_link_libraries(${COMPONENT_LIB} PUBLIC 
    "-Wl,--start-group"
    "${LV_LIB}"
    "${FT_LIB}"
    "-Wl,--end-group"
)

target_include_directories(${COMPONENT_LIB} PUBLIC 
    "${LVGL_ROOT}/include"
    "${FREETYPE_ROOT}/include/freetype2"
)

target_compile_definitions(${COMPONENT_LIB} PUBLIC 
    LV_CONF_PATH=/opt/lvgl/include/lvgl/lv_conf.h
    LV_CONF_INCLUDE_SIMPLE
)