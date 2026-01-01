idf_component_register(
    SRCS "src/max30102.cpp"
         "src/max30102_task.cpp"
    INCLUDE_DIRS "inc"
    REQUIRES driver I2CDevice freertos
)
