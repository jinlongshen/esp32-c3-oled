
idf_component_register(
    SRCS "src/ssd1306.cpp"
    INCLUDE_DIRS "inc"
    REQUIRES I2CDevice
)

idf_component_get_property(oled_lib oled COMPONENT_LIB)
set_property(TARGET ${oled_lib} PROPERTY EXCLUDE_FROM_ALL TRUE)
