# WiFi BLE Provisioning - Simple Integration

Přidejte BLE provisioning do vašeho projektu pomocí 2 souborů.

## 📦 Co zkopírovat

```
main/
├── wifi_prov_ble.c          ← ZKOPÍROVAT
├── wifi_prov_ble.h          ← ZKOPÍROVAT
└── README_COPY_THIS.md      ← Návod (nemusíte kopírovat)
```

**Jen tyto 2 soubory!** Zkopírujte je do vaší `main/` složky.

## 🚀 Použití

### 1. Zkopírovat soubory

```bash
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

### 3. Použít v kódu

```c
#include "wifi_prov_ble.h"

void app_main(void) {
    // Vaše existující inicializace
    init_nvs();
    init_wifi();
    start_softap_provisioning();  // Vaše SoftAP (pokud máte)
    
    // Přidat BLE (1 řádek!)
    wifi_prov_ble_start("abcd1234", NULL);
    
    // Nyní běží SoftAP + BLE současně!
}
```

## 📖 Dokumentace

Detailní návod: `main/README_COPY_THIS.md`

## 🎯 Co to dělá

- ✅ Přidává BLE provisioning k existujícímu projektu
- ✅ Funguje souběžně s SoftAP
- ✅ Jednoduché API: `wifi_prov_ble_start()`, `wifi_prov_ble_stop()`
- ✅ Jen 2 soubory na zkopírování
- ❌ NEINICIALIZUJE WiFi (musíte mít hotové)

## 📂 Struktura projektu

```
main/
├── wifi_prov_ble.c          ← Zkopírovat do vašeho projektu
├── wifi_prov_ble.h          ← Zkopírovat do vašeho projektu
└── README_COPY_THIS.md      ← Detailní návod

Ostatní soubory v main/:
├── wifi_prov_demo.c         ← Demo aplikace (NEKOPÍROVAT)
├── example_*.c              ← Příklady (NEKOPÍROVAT)
└── ...                      ← Ostatní (NEKOPÍROVAT)
```

## 📄 Licence

Unlicense / CC0-1.0 - Použijte jak chcete!
