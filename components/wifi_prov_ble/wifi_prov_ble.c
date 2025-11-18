/*
 * WiFi BLE Provisioning Component Implementation
 */

#include "wifi_prov_ble.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_ble.h"

#define TAG "wifi_prov_ble"

static bool s_ble_active = false;
static char s_service_name[32] = {0};

/* Generate service name from MAC address */
static void generate_service_name(char *name, size_t max_len)
{
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    snprintf(name, max_len, "PROV_%02X%02X%02X", mac[3], mac[4], mac[5]);
}

/* Public API */

esp_err_t wifi_prov_ble_start(const wifi_prov_ble_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (s_ble_active) {
        ESP_LOGW(TAG, "BLE provisioning already active");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Starting BLE provisioning...");
    
    /* Generate or use provided service name */
    if (config->service_name) {
        strncpy(s_service_name, config->service_name, sizeof(s_service_name) - 1);
    } else {
        generate_service_name(s_service_name, sizeof(s_service_name));
    }
    
    ESP_LOGI(TAG, "BLE Service Name: %s", s_service_name);
    
    /* Configure BLE provisioning */
    wifi_prov_mgr_config_t prov_config = {
        .scheme = wifi_prov_scheme_ble,
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
    };
    
    /* Initialize provisioning manager */
    esp_err_t err = wifi_prov_mgr_init(prov_config);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to initialize provisioning manager: %s", esp_err_to_name(err));
        return err;
    }
    
    /* Start provisioning service */
    err = wifi_prov_mgr_start_provisioning(
        WIFI_PROV_SECURITY_1,
        config->pop,
        s_service_name,
        config->service_key
    );
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start provisioning: %s", esp_err_to_name(err));
        wifi_prov_mgr_deinit();
        return err;
    }
    
    s_ble_active = true;
    ESP_LOGI(TAG, "BLE provisioning started successfully");
    ESP_LOGI(TAG, "QR URL: https://espressif.github.io/esp-jumpstart/qrcode.html"
             "{\"ver\":\"v1\",\"name\":\"%s\",\"pop\":\"%s\",\"transport\":\"ble\"}",
             s_service_name, config->pop);
    
    return ESP_OK;
}

esp_err_t wifi_prov_ble_stop(void)
{
    if (!s_ble_active) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Stopping BLE provisioning...");
    
    wifi_prov_mgr_stop_provisioning();
    wifi_prov_mgr_deinit();
    
    s_ble_active = false;
    ESP_LOGI(TAG, "BLE provisioning stopped");
    
    return ESP_OK;
}

bool wifi_prov_ble_is_active(void)
{
    return s_ble_active;
}

esp_err_t wifi_prov_ble_get_service_name(char *name, size_t max_len)
{
    if (!name || max_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    strncpy(name, s_service_name, max_len - 1);
    name[max_len - 1] = '\0';
    
    return ESP_OK;
}
