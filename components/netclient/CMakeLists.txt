idf_component_register(
    SRCS "src/netclient.cpp"
    INCLUDE_DIRS "inc"
    REQUIRES esp_http_client esp-tls
)
