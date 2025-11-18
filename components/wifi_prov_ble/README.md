# WiFi BLE Provisioning Component

**Čistá komponenta** pro přidání BLE provisioning do existujícího projektu s SoftAP provisioning.

## 🎯 Co to je

Toto **NENÍ** kompletní provisioning řešení. Je to **addon** který přidáte k vaší existující implementaci.

```
Váš existující projekt:
├── SoftAP provisioning ✅ (už máte)
├── WiFi initialization ✅ (už máte)
├── Event handling ✅ (už máte)
└── + BLE provisioning ← toto přidáváte
```

## 📦 Struktura komponenty

```
components/wifi_prov_ble/
├── CMakeLists.txt
├── include/
│   └── wifi_prov_ble.h
└── wifi_prov_ble.c
```

## 🚀 Integrace do existujícího projektu

### Krok 1: Zkopírovat komponentu

```bash
cp -r components/wifi_prov_ble /path/to/your/project/components/
```

### Krok 2: Přidat do vašeho main/CMakeLists.txt

```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES wifi_prov_ble  # ← Přidat toto
)
```

### Krok 3: Použít ve vašem kódu

```c
#include "wifi_prov_ble.h"

void app_main(void)
{
    // Vaše existující kód...
    init_nvs();
    init_wifi();
    init_softap_provisioning(); // Vaše SoftAP
    
    // PŘIDAT BLE provisioning
    wifi_prov_ble_config_t ble_config = WIFI_PROV_BLE_CONFIG_DEFAULT();
    wifi_prov_ble_start(&ble_config);
    
    // Nyní běží OBA: SoftAP + BLE
}
```

## 💡 Případy použití

### Případ 1: Souběh SoftAP + BLE

```c
// Vaš existující SoftAP provisioning
start_softap_provisioning();

// Přidat BLE
wifi_prov_ble_config_t config = {
    .pop = "abcd1234",
    .service_name = NULL,  // Auto-generate
    .service_key = NULL,
};
wifi_prov_ble_start(&config);

// Oba běží současně!
// První kdo pošle credentials vyhrává
```

### Případ 2: Uživatel si vybírá

```c
// Tlačítko A: SoftAP
if (button_a_pressed()) {
    start_softap_provisioning();
}

// Tlačítko B: BLE
if (button_b_pressed()) {
    wifi_prov_ble_start(&config);
}
```

### Případ 3: Timeout fallback

```c
// Začít s BLE
wifi_prov_ble_start(&config);

// Po 5 minutách přepnout na SoftAP
vTaskDelay(pdMS_TO_TICKS(5 * 60 * 1000));

if (!is_provisioned()) {
    wifi_prov_ble_stop();
    start_softap_provisioning();
}
```

## 🔧 API Reference

### wifi_prov_ble_start()

Spustí BLE provisioning.

**⚠️ Předpoklady:**
- WiFi musí být inicializovaný (`esp_wifi_init()`)
- Event loop musí existovat
- NVS musí být inicializovaný

```c
wifi_prov_ble_config_t config = {
    .pop = "security_key",
    .service_name = "MyDevice_123",  // nebo NULL pro auto
    .service_key = NULL,
};
esp_err_t err = wifi_prov_ble_start(&config);
```

**Vrací:**
- `ESP_OK` - Úspěch
- `ESP_ERR_INVALID_ARG` - Špatný config
- Jiné - Chyba provisioning manageru

### wifi_prov_ble_stop()

Zastaví BLE provisioning.

```c
wifi_prov_ble_stop();
```

**Použití:**
- Když SoftAP credentials dorazí
- Při timeoutu
- Při přepínání režimů

### wifi_prov_ble_is_active()

Zkontroluje jestli BLE běží.

```c
if (wifi_prov_ble_is_active()) {
    ESP_LOGI(TAG, "BLE is running");
}
```

### wifi_prov_ble_get_service_name()

Získá BLE service name.

```c
char name[32];
wifi_prov_ble_get_service_name(name, sizeof(name));
ESP_LOGI(TAG, "BLE name: %s", name);
```

## 🔄 Integrace s vaším event handlerem

```c
static void your_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_PROV_EVENT) {
        switch (event_id) {
            case WIFI_PROV_CRED_RECV:
                ESP_LOGI(TAG, "Credentials received!");
                
                // Zastavit druhý transport
                if (wifi_prov_ble_is_active()) {
                    wifi_prov_ble_stop();
                }
                // nebo
                if (softap_is_active()) {
                    stop_softap_provisioning();
                }
                break;
                
            case WIFI_PROV_END:
                // Provisioning hotový
                break;
        }
    }
}
```

## ⚙️ Konfigurace

### Minimální (výchozí)

```c
wifi_prov_ble_config_t config = WIFI_PROV_BLE_CONFIG_DEFAULT();
wifi_prov_ble_start(&config);
```

Vytvoří:
- Service name: `PROV_XXXXXX` (z MAC adresy)
- Security: WIFI_PROV_SECURITY_1
- PoP: `abcd1234`

### Vlastní

```c
wifi_prov_ble_config_t config = {
    .pop = "MyDevice2024!",
    .service_name = "SmartLight_001",
    .service_key = NULL,
};
```

## 🎨 Příklad: Kompletní integrace

```c
#include "wifi_prov_ble.h"

// Vaše existující proměnné
static bool softap_active = false;
static bool ble_active = false;

void app_main(void)
{
    // 1. Vaše existující inicializace
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // 2. Registrovat event handler
    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    
    // 3. Zkontrolovat provisioning status
    bool provisioned = false;
    wifi_prov_mgr_is_provisioned(&provisioned);
    
    if (!provisioned) {
        ESP_LOGI(TAG, "Not provisioned - starting dual mode");
        
        // Start SoftAP (vaše existující kód)
        your_softap_start();
        softap_active = true;
        
        // Start BLE (nová komponenta)
        wifi_prov_ble_config_t ble_cfg = WIFI_PROV_BLE_CONFIG_DEFAULT();
        wifi_prov_ble_start(&ble_cfg);
        ble_active = true;
        
        ESP_LOGI(TAG, "Both SoftAP and BLE active - user can choose!");
    } else {
        ESP_LOGI(TAG, "Already provisioned, connecting...");
        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_start();
    }
}

static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_PROV_EVENT) {
        if (event_id == WIFI_PROV_CRED_RECV) {
            // Credentials přijaty!
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
}
```

## 🐛 Troubleshooting

### "Failed to initialize provisioning manager"

**Příčina:** Provisioning manager už je inicializovaný vaším SoftAP kódem.

**Řešení:** Komponenta detekuje `ESP_ERR_INVALID_STATE` a pokračuje dál.

### "BLE nefunguje"

**Příčina:** Bluetooth není nakonfigurován v menuconfig.

**Řešení:**
```bash
idf.py menuconfig
→ Component config → Bluetooth → [*] Bluetooth
```

### "Out of memory"

**Příčina:** BLE + SoftAP zabírá hodně RAM.

**Řešení:** Použijte jen jednu metodu, nebo zvyšte heap.

## 📝 Poznámky

- ✅ **Neobsahuje** `app_main()` - není to aplikace
- ✅ **Neinicializuje** WiFi - předpokládá že už je
- ✅ **Nespravuje** event loop - používá váš existující
- ✅ **Jen přidává** BLE transport k provisioning manageru
- ✅ **Lze kombinovat** s jakoukoliv existující implementací

## 📄 Licence

Unlicense / CC0-1.0 - Použijte jak chcete!
