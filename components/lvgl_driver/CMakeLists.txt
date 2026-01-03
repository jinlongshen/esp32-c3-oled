# Path to host‑built LVGL
set(LVGL_INCLUDE "/opt/lvgl/include")
set(LVGL_LIB_DIR "/opt/lvgl/lib")
set(LVGL_CONF_PATH /opt/lvgl/include/lvgl/lv_conf.h)

idf_component_register(
    SRCS
        "src/lvgl_driver.cpp"
    INCLUDE_DIRS
        "inc"
    REQUIRES
        oled
)

# Make LVGL use the correct lv_conf.h
# IMPORTANT: no escaped quotes, no surrounding quotes
target_compile_definitions(${COMPONENT_LIB} PUBLIC
    LV_CONF_PATH=${LVGL_CONF_PATH}
)

# Add LVGL include directory
target_include_directories(${COMPONENT_LIB} PUBLIC
    ${LVGL_INCLUDE}
)

# Link LVGL static library
target_link_libraries(${COMPONENT_LIB} PUBLIC
    ${LVGL_LIB_DIR}/liblvgl.a
)
