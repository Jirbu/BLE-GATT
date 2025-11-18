# WiFi BLE Provisioning - Ready to Copy

## 📋 Co zkopírovat do vašeho projektu

Zkopírujte **POUZE TYTO 2 SOUBORY** do vaší `main/` složky:

```
wifi_prov_ble.c
wifi_prov_ble.h
```

**To je vše!**

## 🚀 Jak použít

### 1. Zkopírovat soubory

```bash
# Z tohoto projektu do vašeho:
cp main/wifi_prov_ble.c /path/to/your/project/main/
cp main/wifi_prov_ble.h /path/to/your/project/main/
```

### 2. Přidat do vašeho main/CMakeLists.txt

```cmake
idf_component_register(
    SRCS "main.c" "wifi_prov_ble.c"    # ← Přidat wifi_prov_ble.c
    INCLUDE_DIRS "."
    REQUIRES wifi_provisioning protocomm protobuf-c bt esp_wifi
)
```

### 3. Použít ve vašem main.c

```c
#include "wifi_prov_ble.h"

void app_main(void)
{
    // Vaše existující inicializace
    ESP_ERROR_CHECK(nvs_flash_init());
    
    // Vaše WiFi init
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    
    // Vaše SoftAP provisioning (pokud máte)
    start_softap_provisioning();
    
    // PŘIDAT BLE provisioning (1 řádek!)
    wifi_prov_ble_start("abcd1234", NULL);
    
    // Nyní běží SoftAP + BLE současně!
    // První kdo pošle credentials vyhrává
}
```

## 🎯 API - Jednoduché!

### wifi_prov_ble_start()

```c
// Auto-generated name (PROV_XXXXXX z MAC adresy)
wifi_prov_ble_start("abcd1234", NULL);

// Custom name
wifi_prov_ble_start("mykey123", "MyDevice_001");
```

### wifi_prov_ble_stop()

```c
// Zastavit BLE když přijdou SoftAP credentials
if (softap_credentials_received) {
    wifi_prov_ble_stop();
}
```

### wifi_prov_ble_is_active()

```c
if (wifi_prov_ble_is_active()) {
    ESP_LOGI(TAG, "BLE is running");
}
```

## 📱 Mobilní aplikace

Stáhněte **ESP BLE Provisioning** app:
- Android: [Google Play](https://play.google.com/store/apps/details?id=com.espressif.provble)
- iOS: [App Store](https://apps.apple.com/app/esp-ble-provisioning/id1473590141)

Nebo naskenujte QR kód z terminálu.

## 💡 Příklad: Kombinace SoftAP + BLE

```c
#include "wifi_prov_ble.h"

static bool softap_active = false;
static bool ble_active = false;

void app_main(void)
{
    // Init
    nvs_flash_init();
    init_wifi();
    
    // Event handler
    esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &prov_event_handler, NULL);
    
    // Check if provisioned
    bool provisioned = false;
    wifi_prov_mgr_is_provisioned(&provisioned);
    
    if (!provisioned) {
        ESP_LOGI(TAG, "Not provisioned - starting both modes");
        
        // Start SoftAP
        your_softap_start();
        softap_active = true;
        
        // Start BLE
        wifi_prov_ble_start("abcd1234", NULL);
        ble_active = true;
        
        ESP_LOGI(TAG, "User can choose: SoftAP or BLE");
    } else {
        ESP_LOGI(TAG, "Already provisioned");
        esp_wifi_start();
    }
}

// Event handler - zastavit druhý režim když přijdou credentials
static void prov_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_id == WIFI_PROV_CRED_RECV) {
        ESP_LOGI(TAG, "Credentials received!");
        
        // Zastavit druhý transport
        if (ble_active) {
            wifi_prov_ble_stop();
            ble_active = false;
        }
        if (softap_active) {
            your_softap_stop();
            softap_active = false;
        }
    }
}
```

## ⚠️ Důležité

**Předpoklady (musí být hotové PŘED voláním wifi_prov_ble_start):**
- ✅ NVS inicializované
- ✅ WiFi inicializované
- ✅ Event loop vytvořený

## 🐛 Troubleshooting

### "Failed to init prov mgr: ESP_ERR_INVALID_STATE"

**OK!** Provisioning manager už je inicializovaný (pravděpodobně vaším SoftAP kódem). Funkce to detekuje a pokračuje dál.

### "BLE se nezobrazuje"

Zkontrolujte menuconfig:
```bash
idf.py menuconfig
→ Component config → Bluetooth → [*] Bluetooth
```

### "Out of memory"

BLE + SoftAP zabírá hodně RAM. Použijte jen jednu metodu nebo zvyšte heap.

## ✅ Checklist před použitím

- [ ] Zkopíroval jsem `wifi_prov_ble.c` do `main/`
- [ ] Zkopíroval jsem `wifi_prov_ble.h` do `main/`
- [ ] Přidal jsem `wifi_prov_ble.c` do CMakeLists.txt
- [ ] Přidal jsem REQUIRES: `wifi_provisioning protocomm protobuf-c bt`
- [ ] Volám `wifi_prov_ble_start()` PO inicializaci WiFi
- [ ] Bluetooth je zapnutý v menuconfig

## 📄 Licence

Unlicense / CC0-1.0 - Použijte jak chcete!
