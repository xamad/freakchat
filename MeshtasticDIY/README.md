# MeshtasticDIY - ESP32-S3 + DX-LR-30 + Display 2" SPI

Chat LoRa per ESP32-S3 WROOM con modulo DX-LR-30 (SX1262) e display SPI 2".
Usa impostazioni radio compatibili con Meshtastic (preset LONG_FAST).

## Hardware Richiesto

- **ESP32-S3 WROOM** (o DevKitC-1)
- **DX-LR-30** (modulo LoRa SX1262)
- **Display SPI 2"** (ST7789 o ILI9341, 240x320)
- Antenna 868MHz

## Schema Collegamento

### LoRa DX-LR-30 (SPI HSPI)

```
ESP32-S3            DX-LR-30
========            ========
GPIO10  ─────────►  Pin 9  (NSS)
GPIO12  ─────────►  Pin 10 (SCK)
GPIO13  ◄─────────  Pin 11 (MISO)
GPIO11  ─────────►  Pin 12 (MOSI)
GPIO2   ◄─────────  Pin 13 (DIO1)
GPIO3   ─────────►  Pin 17 (RST)
GPIO4   ◄─────────  Pin 21 (BUSY)
3.3V    ─────────►  Pin 1  (VCC)
GND     ─────────►  Pin 2  (GND)
```

### Display SPI 2" (SPI VSPI)

```
ESP32-S3            Display
========            =======
GPIO35  ─────────►  SCK (CLK)
GPIO37  ─────────►  MOSI (SDA/DIN)
GPIO36  ─────────►  CS
GPIO38  ─────────►  DC (RS)
GPIO39  ─────────►  RST
GPIO40  ─────────►  BL (Backlight)
3.3V    ─────────►  VCC
GND     ─────────►  GND
```

### Pinout DX-LR-30

```
        ┌─────────────────┐
   VCC ─┤ 1            24 ├─ NC
   GND ─┤ 2            23 ├─ NC
    NC ─┤ 3            22 ├─ NC
    NC ─┤ 4            21 ├─ BUSY ◄── GPIO4
    NC ─┤ 5            20 ├─ NC
    NC ─┤ 6            19 ├─ NC
   ANT ─┤ 7            18 ├─ GND
    NC ─┤ 8            17 ├─ RST  ◄── GPIO3
   NSS ─┤ 9  ◄─GPIO10  16 ├─ NC
   SCK ─┤ 10 ◄─GPIO12  15 ├─ NC
  MISO ─┤ 11 ──►GPIO13 14 ├─ DIO2
  MOSI ─┤ 12 ◄─GPIO11  13 ├─ DIO1 ◄── GPIO2
        └─────────────────┘
```

## Setup Arduino IDE

### 1. Installa ESP32-S3

1. **File** → **Preferences**
2. Aggiungi URL:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. **Tools** → **Board Manager** → Installa **esp32**

### 2. Installa Librerie

**Sketch** → **Include Library** → **Manage Libraries**:

1. **RadioLib** by Jan Gromes
2. **TFT_eSPI** by Bodmer

### 3. Configura TFT_eSPI

**IMPORTANTE:** Copia `User_Setup.h` nella cartella della libreria:
```
Arduino/libraries/TFT_eSPI/User_Setup.h
```

Oppure modifica il file esistente con i pin:
```cpp
#define ST7789_DRIVER
#define TFT_WIDTH  240
#define TFT_HEIGHT 320
#define TFT_MOSI 37
#define TFT_SCLK 35
#define TFT_CS   36
#define TFT_DC   38
#define TFT_RST  39
#define TFT_BL   40
#define TFT_MISO -1
```

### 4. Configura Board

1. **Tools** → **Board** → **ESP32S3 Dev Module**
2. **USB CDC On Boot** → **Enabled**
3. **USB Mode** → **Hardware CDC and JTAG**
4. **Flash Size** → **4MB** (o quello del tuo modulo)
5. **PSRAM** → **Disabled** (o OPI se disponibile)

### 5. Upload

1. Collega ESP32-S3 via USB
2. Tieni premuto **BOOT** + premi **RESET** (se necessario)
3. **Sketch** → **Upload**
4. Apri **Serial Monitor** a 115200

## Uso

### Comandi (via Serial Monitor o Bluetooth)

| Comando | Descrizione |
|---------|-------------|
| `/nick <nome>` | Imposta nickname |
| `/ping` | Invia ping di test |
| `/info` | Mostra info radio |
| `/clear` | Pulisce chat |
| `<testo>` | Invia messaggio |

### Interfaccia Display

```
┌────────────────────────────────────┐
│ MeshtasticDIY      868.0 MHz SF11  │  ← Header
├────────────────────────────────────┤
│ * Radio OK - 868.0 MHz             │  ← System msg
│ <Mario> Ciao!                      │  ← Received
│ <Tu> Ciao Mario!                   │  ← Sent
│                                    │
│                                    │
├────────────────────────────────────┤
│ > messaggio_                       │  ← Input
└────────────────────────────────────┘
```

## Configurazione LoRa

Preset **LONG_FAST** (Meshtastic default):

| Parametro | Valore |
|-----------|--------|
| Frequenza | 868.0 MHz |
| Bandwidth | 250 kHz |
| Spreading | SF11 |
| Coding Rate | 4/5 |
| Sync Word | 0x2B |
| TX Power | 14 dBm |

Per cambiare preset, modifica le define nel `.ino`:
```cpp
// MEDIUM_FAST (più veloce, meno range)
#define LORA_BANDWIDTH      250.0
#define LORA_SPREADING      9
#define LORA_CODING_RATE    5

// LONG_SLOW (massimo range)
#define LORA_BANDWIDTH      125.0
#define LORA_SPREADING      12
#define LORA_CODING_RATE    8
```

## Tastiera Smartphone (Bluetooth)

Per usare lo smartphone come tastiera:

1. Aggiungi al codice:
```cpp
#include <BluetoothSerial.h>
BluetoothSerial SerialBT;

void setup() {
    SerialBT.begin("MeshtasticDIY");
    // ...
}

void loop() {
    // Check Bluetooth input
    if (SerialBT.available()) {
        // same as Serial handling
    }
}
```

2. Scarica app "Serial Bluetooth Terminal" su smartphone
3. Connettiti a "MeshtasticDIY"
4. Invia comandi!

## Troubleshooting

### Display bianco/nero
- Controlla `User_Setup.h` copiato correttamente
- Prova `#define TFT_INVERSION_ON`
- Verifica cablaggio SPI display

### LoRa Error -2 (Chip not found)
- Controlla cablaggio SPI LoRa
- Verifica alimentazione 3.3V

### LoRa Error -6 (Timeout)
- Controlla BUSY (GPIO4 → Pin 21)
- Controlla RST (GPIO3 → Pin 17)

### Nessuna ricezione
- Antenna collegata?
- Stessa frequenza/sync word degli altri dispositivi?

## Note

- Questo firmware **non** è Meshtastic completo (no protobuf)
- Comunica solo con altri dispositivi MeshtasticDIY
- Per Meshtastic completo, usa firmware ufficiale con variant custom

## License

MIT License
