idf_component_register(SRCS "src/provision.cpp"
                    INCLUDE_DIRS "inc"
                    REQUIRES esp_driver_gpio esp_event esp_netif esp_wifi nvs_flash 
                    PRIV_REQUIRES wifi_provisioning bt protocomm lvgl)