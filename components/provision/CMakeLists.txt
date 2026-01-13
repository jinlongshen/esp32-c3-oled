idf_component_register(
    SRCS "src/provision.cpp"
    INCLUDE_DIRS "inc"
    PRIV_REQUIRES 
        esp_driver_gpio
        esp_wifi nvs_flash
        wifi_provisioning
)