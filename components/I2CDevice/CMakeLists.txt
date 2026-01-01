idf_component_register(
    SRCS "src/I2CBus.cpp"
         "src/I2CDevice.cpp"
    INCLUDE_DIRS "inc"
    REQUIRES esp_driver_i2c
)
