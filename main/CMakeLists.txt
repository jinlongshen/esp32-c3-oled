
idf_component_register(
    SRCS "main.cpp" 
    INCLUDE_DIRS "." 
    REQUIRES I2CDevice driver oled esp_driver_i2c esp_driver_gpio
)

# idf_component_get_property(main_lib main COMPONENT_LIB)
# set_property(TARGET ${main_lib} PROPERTY EXCLUDE_FROM_ALL TRUE)