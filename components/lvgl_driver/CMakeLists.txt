idf_component_register(
    SRCS "src/lvgl_driver.cpp"
    INCLUDE_DIRS "inc"
    PRIV_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}"
    REQUIRES oled lvgl
)

