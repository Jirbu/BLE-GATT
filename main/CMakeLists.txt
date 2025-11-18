idf_component_register(SRCS "wifi_prov_ble.c" "wifi_prov_demo.c"
                       INCLUDE_DIRS "."
                       REQUIRES wifi_provisioning protocomm protobuf-c bt esp_wifi nvs_flash esp_event esp_netif esp_driver_gpio)
