
idf_component_register(
    SRCS "main.cpp" 
    INCLUDE_DIRS "." 
    PRIV_INCLUDE_DIRS "/opt/freetype/include/freetype2"
    REQUIRES I2CDevice driver oled esp_driver_i2c esp_driver_gpio
)

target_link_libraries(${COMPONENT_LIB} "/opt/freetype/lib/libfreetype.a" )

# idf_component_get_property(main_lib main COMPONENT_LIB)
# set_property(TARGET ${main_lib} PROPERTY EXCLUDE_FROM_ALL TRUE)