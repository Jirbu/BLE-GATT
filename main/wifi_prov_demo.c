/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

/****************************************************************************
*
* This demo implements WiFi provisioning over BLE using the WiFi Provisioning Manager.
* It allows mobile apps to send WiFi credentials to ESP32 via BLE.
* The device will connect to the provided WiFi network and save credentials for future use.
*
****************************************************************************/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gatt_common_api.h"

#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_ble.h"

#define PROV_TAG "wifi_prov_mgr"

#define PROV_QR_VERSION         "v1"
#define PROV_TRANSPORT_BLE      "ble"
#define QRCODE_BASE_URL         "https://espressif.github.io/esp-jumpstart/qrcode.html"

/* Signal Wi-Fi events on this event-group */
const int WIFI_CONNECTED_EVENT = BIT0;
static EventGroupHandle_t wifi_event_group;

#define POP_TYPE_STRING     0
#define POP_TYPE_MAC        1

// Enable reset button - hold this GPIO low for 5 seconds after boot to erase credentials
// WARNING: GPIO34 is input-only and has NO internal pull-up!
// You MUST add external 10kΩ pull-up resistor between GPIO34 and 3.3V
// Connect button between GPIO34 and GND
#define PROV_RESET_GPIO     GPIO_NUM_34
#define RESET_BUTTON_HOLD_TIME_MS   5000

// LED indicator for reset confirmation
#define RESET_LED_GPIO      GPIO_NUM_13
#define LED_BLINK_COUNT     10
#define LED_BLINK_DELAY_MS  50

#ifdef PROV_RESET_GPIO
#include "driver/gpio.h"
#endif

static const char *pop = "abcd1234";
static const char *service_key  = NULL;

/* Event handler for catching system events */
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data);

/* Handlers for getting QR code */
static void get_device_service_name(char *service_name, size_t max);
static void wifi_prov_print_qr(const char *name, const char *username, const char *pop, const char *transport);

/* WiFi event handler */
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    static int retries = 0;
    if (event_base == WIFI_PROV_EVENT) {
        switch (event_id) {
            case WIFI_PROV_START:
                ESP_LOGI(PROV_TAG, "Provisioning started");
                break;
            case WIFI_PROV_CRED_RECV: {
                wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
                ESP_LOGI(PROV_TAG, "Received Wi-Fi credentials"
                         "\n\tSSID     : %s\n\tPassword : %s",
                         (const char *) wifi_sta_cfg->ssid,
                         (const char *) wifi_sta_cfg->password);
                break;
            }
            case WIFI_PROV_CRED_FAIL: {
                wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
                ESP_LOGE(PROV_TAG, "Provisioning failed!\n\tReason : %s"
                         "\n\tPlease reset to factory and retry provisioning",
                         (*reason == WIFI_PROV_STA_AUTH_ERROR) ?
                         "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
                retries++;
                if (retries >= 5) {
                    ESP_LOGI(PROV_TAG, "Failed to connect with provisioned AP, reseting provisioned credentials");
                    wifi_prov_mgr_reset_sm_state_on_failure();
                    retries = 0;
                }
                break;
            }
            case WIFI_PROV_CRED_SUCCESS:
                ESP_LOGI(PROV_TAG, "Provisioning successful");
                retries = 0;
                break;
            case WIFI_PROV_END:
                /* De-initialize manager once provisioning is finished */
                wifi_prov_mgr_deinit();
                break;
            default:
                break;
        }
    } else if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(PROV_TAG, "Disconnected. Connecting to the AP again...");
                esp_wifi_connect();
                break;
            default:
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(PROV_TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        /* Signal main application to continue execution */
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_EVENT);
    }
}

static void get_device_service_name(char *service_name, size_t max)
{
    uint8_t eth_mac[6];
    const char *ssid_prefix = "PROV_";
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(service_name, max, "%s%02X%02X%02X",
             ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
}

static void wifi_prov_print_qr(const char *name, const char *username, const char *pop, const char *transport)
{
    if (!name || !transport) {
        ESP_LOGW(PROV_TAG, "Cannot generate QR code payload. Data missing.");
        return;
    }
    char payload[150] = {0};
    if (pop) {
        snprintf(payload, sizeof(payload), 
                 "{\"ver\":\"%s\",\"name\":\"%s\""
                 ",\"pop\":\"%s\",\"transport\":\"%s\"}",
                 PROV_QR_VERSION, name, pop, transport);
    } else {
        snprintf(payload, sizeof(payload), 
                 "{\"ver\":\"%s\",\"name\":\"%s\""
                 ",\"transport\":\"%s\"}",
                 PROV_QR_VERSION, name, transport);
    }
    ESP_LOGI(PROV_TAG, "Scan this QR code from the provisioning application for Provisioning.");
    ESP_LOGI(PROV_TAG, "%s%s", QRCODE_BASE_URL, payload);
    ESP_LOGI(PROV_TAG, "If QR code is not visible, copy paste the below URL in a browser.\n%s%s", QRCODE_BASE_URL, payload);
}

static void init_wifi(void)
{
    /* Initialize TCP/IP */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Initialize the event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_event_group = xEventGroupCreate();

    /* Register our event handler for Wi-Fi, IP and Provisioning related events */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    /* Initialize Wi-Fi including netif with default config */
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
}

void app_main(void)
{
    /* Initialize NVS partition */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /* NVS partition was truncated and needs to be erased
         * Retry nvs_flash_init */
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    ESP_LOGI(PROV_TAG, "Starting WiFi Provisioning Demo");
    ESP_LOGI(PROV_TAG, "Free memory: %ld bytes", esp_get_free_heap_size());

#ifdef PROV_RESET_GPIO
    /* Configure reset button GPIO */
    gpio_reset_pin(PROV_RESET_GPIO);
    gpio_set_direction(PROV_RESET_GPIO, GPIO_MODE_INPUT);
    // GPIO34 is input-only, cannot enable internal pull-up
    // External pull-up resistor is required!
    
    /* Check if button is held for 5 seconds */
    ESP_LOGI(PROV_TAG, "Checking for reset button on GPIO%d...", PROV_RESET_GPIO);
    ESP_LOGI(PROV_TAG, "Hold button for %d seconds to erase WiFi credentials", RESET_BUTTON_HOLD_TIME_MS/1000);
    
    int button_pressed_count = 0;
    for (int i = 0; i < RESET_BUTTON_HOLD_TIME_MS / 100; i++) {
        if (gpio_get_level(PROV_RESET_GPIO) == 0) {
            button_pressed_count++;
            if (button_pressed_count == 10) { // After 1 second
                ESP_LOGW(PROV_TAG, "Button detected, keep holding...");
            }
            if (button_pressed_count == 30) { // After 3 seconds
                ESP_LOGW(PROV_TAG, "Almost there, keep holding...");
            }
        } else {
            if (button_pressed_count > 0) {
                ESP_LOGI(PROV_TAG, "Button released too early, reset cancelled");
            }
            button_pressed_count = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    if (button_pressed_count >= (RESET_BUTTON_HOLD_TIME_MS / 100)) {
        ESP_LOGW(PROV_TAG, "Reset button held for %d seconds - ERASING WiFi credentials!", RESET_BUTTON_HOLD_TIME_MS/1000);
        
        // Initialize WiFi first (needed for wifi_prov_mgr functions)
        init_wifi();
        
        // Initialize provisioning manager
        wifi_prov_mgr_config_t config = {
            .scheme = wifi_prov_scheme_ble,
            .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
        };
        esp_err_t err = wifi_prov_mgr_init(config);
        if (err == ESP_OK) {
            wifi_prov_mgr_reset_provisioning();
            ESP_LOGI(PROV_TAG, "Credentials erased successfully!");
        }
        
        // Configure LED for indication
        gpio_reset_pin(RESET_LED_GPIO);
        gpio_set_direction(RESET_LED_GPIO, GPIO_MODE_OUTPUT);
        
        // Blink LED 10 times to indicate successful erase
        ESP_LOGI(PROV_TAG, "Blinking LED on GPIO%d to confirm erase...", RESET_LED_GPIO);
        for (int i = 0; i < LED_BLINK_COUNT; i++) {
            gpio_set_level(RESET_LED_GPIO, 1);
            vTaskDelay(pdMS_TO_TICKS(LED_BLINK_DELAY_MS));
            gpio_set_level(RESET_LED_GPIO, 0);
            vTaskDelay(pdMS_TO_TICKS(LED_BLINK_DELAY_MS));
        }
        
        ESP_LOGI(PROV_TAG, "Rebooting in 3 seconds...");
        vTaskDelay(pdMS_TO_TICKS(5000));
        esp_restart();
    } else {
        ESP_LOGI(PROV_TAG, "No reset button press detected, continuing normal boot");
    }
#endif

    /* Initialize WiFi including netif with default config */
    init_wifi();
    
    ESP_LOGI(PROV_TAG, "WiFi initialized successfully");

    /* Configuration for the provisioning manager */
    wifi_prov_mgr_config_t config = {
        .scheme = wifi_prov_scheme_ble,
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
    };

    /* Initialize provisioning manager with the configuration parameters */
    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));
    
    ESP_LOGI(PROV_TAG, "Provisioning manager initialized");

    bool provisioned = false;
    /* Let's find out if the device is provisioned */
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
    
    ESP_LOGI(PROV_TAG, "Provisioning status: %s", provisioned ? "ALREADY PROVISIONED" : "NOT PROVISIONED - STARTING BLE");

    /* If device is not yet provisioned start provisioning service */
    if (!provisioned) {
        ESP_LOGI(PROV_TAG, "Starting provisioning");

        /* What is the Device Service Name that we want
         * This translates to :
         *     - Wi-Fi SSID when scheme is wifi_prov_scheme_softap
         *     - BLE Device Name when scheme is wifi_prov_scheme_ble
         */
        char service_name[12];
        get_device_service_name(service_name, sizeof(service_name));

        /* What is the security level that we want (0 or 1):
         *      - WIFI_PROV_SECURITY_0 is simply plain text communication.
         *      - WIFI_PROV_SECURITY_1 is secure communication which consists of secure handshake
         *          using X25519 key exchange and proof of possession (pop) and AES-CTR
         *          for encryption/decryption of messages.
         */
        wifi_prov_security_t security = WIFI_PROV_SECURITY_1;

        /* Do we want a proof-of-possession (ignored if Security 0) */
        const char *username  = NULL;

        /* Start provisioning service */
        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, pop, service_name, service_key));

        /* The handler for the optional endpoint created by the application.
         * The data received on this endpoint is simply printed in the logs.
         * The endpoint name can be anything of your choice.
         * This call must be made after starting the provisioning, and only if the endpoint
         * has already been created above.
         */

        /* Uncomment the following to wait for the provisioning to finish and then release
         * the resources of the manager. Since in this case de-initialization is triggered
         * by the default event loop handler, we don't need to call the following */
        // wifi_prov_mgr_wait();
        // wifi_prov_mgr_deinit();
        /* Print QR code for provisioning */
        wifi_prov_print_qr(service_name, username, pop, PROV_TRANSPORT_BLE);
    } else {
        ESP_LOGI(PROV_TAG, "Already provisioned, starting Wi-Fi STA");

        /* We don't need the manager as device is already provisioned,
         * so let's release it's resources */
        wifi_prov_mgr_deinit();

        /* Start Wi-Fi station */
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());
    }

    /* Wait for Wi-Fi connection */
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_EVENT, false, true, portMAX_DELAY);

    /* Start main application now */
    ESP_LOGI(PROV_TAG, "WiFi Provisioning completed successfully!");
    ESP_LOGI(PROV_TAG, "Main application can now continue...");
    
    while(1) {
        ESP_LOGI(PROV_TAG, "Hello from ESP32! WiFi is connected.");
        vTaskDelay(pdMS_TO_TICKS(10000)); // Print every 10 seconds
    }
}