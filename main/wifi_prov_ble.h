/*
 * WiFi BLE Provisioning
 * 
 * Simple addon for existing WiFi provisioning.
 * Copy wifi_prov_ble.c + wifi_prov_ble.h to your main/ folder.
 */

#ifndef WIFI_PROV_BLE_H
#define WIFI_PROV_BLE_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start BLE provisioning
 * 
 * Prerequisites (must be done BEFORE calling this):
 * - NVS initialized (nvs_flash_init)
 * - WiFi initialized (esp_wifi_init)
 * - Event loop created
 * 
 * @param pop Proof of possession (security key), e.g. "abcd1234"
 * @param service_name_custom Custom BLE name or NULL for auto-generate
 * @return ESP_OK on success
 * 
 * Example:
 *   wifi_prov_ble_start("abcd1234", NULL);  // Auto name: PROV_XXXXXX
 *   wifi_prov_ble_start("mykey", "MyDevice_001");  // Custom name
 */
esp_err_t wifi_prov_ble_start(const char *pop, const char *service_name_custom);

/**
 * @brief Stop BLE provisioning
 * 
 * Call when:
 * - SoftAP credentials received
 * - Timeout
 * - Switching modes
 */
esp_err_t wifi_prov_ble_stop(void);

/**
 * @brief Check if BLE is active
 * @return true if running
 */
bool wifi_prov_ble_is_active(void);

/**
 * @brief Get BLE service name
 * @return Service name string
 */
const char* wifi_prov_ble_get_name(void);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_PROV_BLE_H */
