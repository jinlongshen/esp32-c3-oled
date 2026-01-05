set(LVGL_BASE_DIR "/opt/lvgl")
set(LVGL_LIB_DIR "${LVGL_BASE_DIR}/lib")

idf_component_register(
    INCLUDE_DIRS "${LVGL_INCLUDE}"
)

target_compile_definitions(${COMPONENT_LIB} INTERFACE
    LV_CONF_PATH=${LVGL_CONF_PATH}
)

target_link_libraries(${COMPONENT_LIB} INTERFACE
    "${LVGL_LIB_DIR}/liblvgl.a"
)
