idf_component_register(
    SRCS "src/I2CBus.cpp"
         "src/I2CDevice.cpp"
    INCLUDE_DIRS "inc"
    REQUIRES driver esp_driver_i2c esp_driver_gpio
)
