idf_component_register(
    SRCS "src/main.cpp"
         "src/Hooks.cpp"
    INCLUDE_DIRS "inc"
    REQUIRES fonts oled lvgl_driver ui
)