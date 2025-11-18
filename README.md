# WiFi BLE Provisioning Component

ESP-IDF komponenta pro přidání BLE provisioning do existujícího projektu.

## 📦 Obsah

```
components/wifi_prov_ble/    ← Zkopírujte celou tuto složku do vašeho projektu
├── CMakeLists.txt
├── README.md                ← Detailní dokumentace
├── include/
│   └── wifi_prov_ble.h
└── wifi_prov_ble.c
```

**To je vše! Pouze 4 soubory.**

## 🚀 Použití v cílovém projektu

### 1. Zkopírovat komponentu

```bash
cp -r components/wifi_prov_ble /path/to/your/project/components/
```

### 2. Přidat do vašeho main/CMakeLists.txt

```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES wifi_prov_ble  # ← Přidat
)
```

### 3. Použít v kódu

```c
#include "wifi_prov_ble.h"

void app_main(void) {
    // Vaše existující inicializace...
    init_nvs();
    init_wifi();
    start_softap_provisioning();  // Vaše SoftAP
    
    // Přidat BLE (1 řádek!)
    wifi_prov_ble_config_t cfg = WIFI_PROV_BLE_CONFIG_DEFAULT();
    wifi_prov_ble_start(&cfg);
    
    // Nyní běží SoftAP + BLE současně!
}
```

## 📖 Dokumentace

Kompletní dokumentace: `components/wifi_prov_ble/README.md`

## 🎯 Co to dělá

- ✅ Přidává BLE transport k existujícímu provisioning
- ✅ Funguje souběžně s SoftAP
- ✅ Čisté API: `start()`, `stop()`, `is_active()`
- ❌ NEOBSAHUJE kompletní aplikaci
- ❌ NEINICIALIZUJE WiFi (předpokládá že už máte)
- ❌ Není to hotová aplikace - jen komponenta!

## 📄 Licence

Unlicense / CC0-1.0
