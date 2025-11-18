idf_component_register(
    SRCS "wifi_prov_ble.c"
    INCLUDE_DIRS "include"
    REQUIRES wifi_provisioning protocomm protobuf-c bt esp_wifi
)
