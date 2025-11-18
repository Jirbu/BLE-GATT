/*
 * WiFi BLE Provisioning Component
 * 
 * Lightweight wrapper for ESP-IDF WiFi provisioning manager with BLE transport.
 * Designed to be integrated alongside existing SoftAP provisioning.
 */

#ifndef WIFI_PROV_BLE_H
#define WIFI_PROV_BLE_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BLE provisioning configuration
 */
typedef struct {
    const char *pop;                /*!< Proof of possession (security key) */
    const char *service_name;       /*!< BLE service name (NULL = auto-generate from MAC) */
    const char *service_key;        /*!< Optional service key */
} wifi_prov_ble_config_t;

/**
 * @brief Default BLE provisioning configuration
 */
#define WIFI_PROV_BLE_CONFIG_DEFAULT() { \
    .pop = "abcd1234", \
    .service_name = NULL, \
    .service_key = NULL, \
}

/**
 * @brief Initialize BLE provisioning (does NOT initialize WiFi!)
 * 
 * Call this AFTER your existing WiFi/provisioning manager initialization.
 * This only adds BLE transport capability.
 * 
 * Prerequisites:
 * - WiFi must be initialized (esp_wifi_init())
 * - Event loop must exist
 * - NVS must be initialized
 * 
 * @param config BLE provisioning configuration
 * @return ESP_OK on success
 */
esp_err_t wifi_prov_ble_start(const wifi_prov_ble_config_t *config);

/**
 * @brief Stop BLE provisioning
 * 
 * Stops BLE advertising and frees resources.
 * Call this when provisioning completes or when switching to SoftAP mode.
 * 
 * @return ESP_OK on success
 */
esp_err_t wifi_prov_ble_stop(void);

/**
 * @brief Check if BLE provisioning is currently active
 * 
 * @return true if BLE provisioning is running
 */
bool wifi_prov_ble_is_active(void);

/**
 * @brief Get BLE service name
 * 
 * @param name Buffer to store name (min 12 bytes)
 * @param max_len Maximum buffer length
 * @return ESP_OK on success
 */
esp_err_t wifi_prov_ble_get_service_name(char *name, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_PROV_BLE_H */
