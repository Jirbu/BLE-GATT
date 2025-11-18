# WiFi Provisioning Demo pro ESP32

Tento projekt byl přepracován z původního BLE GATT Server demo na kompletní WiFi provisioning řešení pomocí BLE.

## Co program dělá

Program implementuje **WiFi provisioning přes BLE**:

1. **Po prvním spuštění** - spustí BLE službu pro provisioning
2. **Mobilní aplikace** se připojí přes BLE a pošle WiFi údaje (SSID + heslo)
3. **ESP32 se připojí** k zadané WiFi síti
4. **Údaje se uloží** do NVS pro příští spuštění
5. **BLE se vypne** pro úsporu paměti
6. **Program pokračuje** s WiFi připojením

## Použití

### 1. Kompilace a nahrání
```bash
idf.py build
idf.py flash monitor
```

### 2. Mobilní aplikace
Pro provisioning použijte některou z oficiálních aplikací:

**Android:**
- [ESP BLE Provisioning](https://play.google.com/store/apps/details?id=com.espressif.provble)

**iOS:**
- [ESP BLE Provisioning](https://apps.apple.com/app/esp-ble-provisioning/id1473590141)

### 3. QR kód
Po spuštění program vypíše QR kód v konzoli, který můžete naskenovat mobilní aplikací pro rychlé připojení.

### 4. Provisioning proces
1. Spusťte mobilní aplikaci
2. Vyhledejte zařízení "PROV_XXXXXX" (kde X jsou poslední 3 bajty MAC adresy)
3. Zadejte **Proof-of-Possession**: `abcd1234`
4. Zadejte WiFi SSID a heslo
5. ESP32 se připojí k WiFi

## Konfigurace

### Bezpečnostní údaje
V souboru `wifi_prov_demo.c`:
```c
static const char *pop = "abcd1234";  // Změňte pro vyšší bezpečnost
```

### BLE název zařízení
Automaticky generovaný jako `PROV_` + posledních 6 hex číslic MAC adresy.

## Partition Table

Projekt používá custom partition table (`partitions.csv`) s:
- **NVS**: 24KB pro ukládání WiFi údajů
- **Factory app**: 2MB pro aplikaci
- **Flash size**: 4MB

## Reset provisioning údajů

Pro smazání uložených WiFi údajů:
```bash
idf.py erase-flash
```

## Ladění

Monitor výstup přes sériový port:
```bash
idf.py monitor
```

Typický výstup:
```
I (512) wifi_prov_mgr: Starting provisioning
I (522) wifi_prov_mgr: Scan this QR code from the provisioning application
I (532) wifi_prov_mgr: https://espressif.github.io/esp-jumpstart/qrcode.html?data=...
```

## Kompatibilita

- **ESP32, ESP32-S3, ESP32-C3** (všechny s BLE)
- **ESP-IDF v5.3+**
- **4MB flash** (minimálně)

## Další informace

- [ESP-IDF WiFi Provisioning dokumentace](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/provisioning/wifi_provisioning.html)
- [Mobilní aplikace source code](https://github.com/espressif/esp-idf-provisioning-android)