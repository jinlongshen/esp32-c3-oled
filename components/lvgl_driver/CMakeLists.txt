idf_component_register(
    SRCS "src/lvgl_display.cpp"
         "src/lvgl_driver.cpp"
         "src/lvgl_tasks.cpp"
    INCLUDE_DIRS "inc"
    PRIV_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}"
    REQUIRES oled lvgl
)

