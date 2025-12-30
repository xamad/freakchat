# Meshtastic Variant: DIY ESP32-S3 + DX-LR-30

Variant Meshtastic personalizzato per ESP32-S3 WROOM con modulo LoRa DX-LR-30 (SX1262) e display SPI 2".

## Hardware Supportato

- **MCU:** ESP32-S3 WROOM (o DevKitC-1)
- **LoRa:** DX-LR-30 (SX1262, 868MHz)
- **Display:** ST7789 2" SPI (320x240)
- **Tastiera:** App Meshtastic via Bluetooth

## Schema Collegamento

### LoRa DX-LR-30 (SPI HSPI)

| ESP32-S3 | DX-LR-30 | Funzione |
|----------|----------|----------|
| GPIO10 | Pin 9 | NSS (CS) |
| GPIO12 | Pin 10 | SCK |
| GPIO13 | Pin 11 | MISO |
| GPIO11 | Pin 12 | MOSI |
| GPIO2 | Pin 13 | DIO1 |
| GPIO3 | Pin 17 | RST |
| GPIO4 | Pin 21 | BUSY |
| 3.3V | Pin 1 | VCC |
| GND | Pin 2 | GND |

### Display SPI 2" (SPI VSPI)

| ESP32-S3 | Display | Funzione |
|----------|---------|----------|
| GPIO35 | SCK | Clock |
| GPIO37 | MOSI | Data |
| GPIO36 | CS | Chip Select |
| GPIO38 | DC | Data/Command |
| GPIO39 | RST | Reset |
| GPIO40 | BL | Backlight |
| 3.3V | VCC | Power |
| GND | GND | Ground |

### I2C (Opzionale per sensori)

| ESP32-S3 | Funzione |
|----------|----------|
| GPIO8 | SDA |
| GPIO9 | SCL |

## Installazione

### 1. Clona il firmware Meshtastic

```bash
git clone --recursive https://github.com/meshtastic/firmware.git
cd firmware
```

### 2. Copia il variant

```bash
# Copia la cartella variant
cp -r /path/to/freakchat/meshtastic-variant/diy-esp32s3-dxlr30 variants/
```

### 3. Aggiungi la configurazione a platformio.ini

Apri `platformio.ini` e aggiungi alla fine il contenuto di `platformio_entry.ini`:

```ini
[env:diy-esp32s3-dxlr30]
extends = esp32s3_base
board = esp32-s3-devkitc-1
board_build.mcu = esp32s3

board_build.flash_mode = dio
board_build.flash_size = 4MB
board_build.partitions = partitions-4MB.csv

board_build.f_cpu = 240000000L
build_flags =
    ${esp32s3_base.build_flags}
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DARDUINO_USB_MODE=1
    -DBOARD_HAS_PSRAM=0
    -I variants/diy-esp32s3-dxlr30
    -DUSE_SX1262
    -DSX126X_CS=10
    -DSX126X_DIO1=2
    -DSX126X_BUSY=4
    -DSX126X_RESET=3
    -DLORA_SCK=12
    -DLORA_MISO=13
    -DLORA_MOSI=11
    -DUSE_ST7789
    -DST7789_CS=36
    -DST7789_RS=38
    -DST7789_SDA=37
    -DST7789_SCK=35
    -DST7789_RESET=39
    -DST7789_BL=40
    -DI2C_SDA=8
    -DI2C_SCL=9
    -DVARIANT_H=\"variants/diy-esp32s3-dxlr30/variant.h\"

lib_deps =
    ${esp32s3_base.lib_deps}
```

### 4. Compila

```bash
# Installa PlatformIO se non l'hai
pip install platformio

# Compila il firmware
pio run -e diy-esp32s3-dxlr30

# Il firmware sarà in:
# .pio/build/diy-esp32s3-dxlr30/firmware.bin
```

### 5. Flash

```bash
# Collega ESP32-S3 via USB
# Tieni premuto BOOT e premi RESET

# Flash con esptool
esptool.py --chip esp32s3 --port /dev/ttyUSB0 \
    --baud 921600 \
    write_flash -z 0x0 \
    .pio/build/diy-esp32s3-dxlr30/firmware.bin

# Oppure usa PlatformIO
pio run -e diy-esp32s3-dxlr30 -t upload
```

## Configurazione Post-Flash

### 1. Prima accensione

Il dispositivo creerà un access point WiFi:
- SSID: `Meshtastic_XXXX`
- Password: nessuna

### 2. Connessione App

1. Scarica l'app **Meshtastic** (Android/iOS)
2. Abilita Bluetooth
3. L'app troverà il dispositivo
4. PIN di pairing: `123456`

### 3. Configurazione Radio

Dall'app, configura:
- **Region:** EU_868 (per Europa)
- **Modem Preset:** LONG_FAST (default)

## Troubleshooting

### Il dispositivo non si avvia
- Verifica alimentazione 3.3V stabile
- Controlla che tutti i pin siano collegati correttamente

### LoRa non funziona
- Controlla cablaggio SPI (NSS, SCK, MOSI, MISO)
- Verifica RST e BUSY
- Antenna collegata?

### Display non funziona
- Verifica User_Setup.h di TFT_eSPI (se usato)
- Controlla pin DC e RST
- Prova a invertire i colori in variant.h

### Bluetooth non trova il dispositivo
- Riavvia il dispositivo
- Disabilita e riabilita Bluetooth sul telefono
- Prova "Forget device" e rifare pairing

## Modifica Pin

Se il tuo cablaggio è diverso, modifica:

1. **variant.h** - Le define dei pin
2. **platformio_entry.ini** - I build_flags

## File Inclusi

```
meshtastic-variant/
├── README.md                    # Questa guida
└── diy-esp32s3-dxlr30/
    ├── variant.h                # Definizioni hardware
    ├── pins_arduino.h           # Pin Arduino
    └── platformio_entry.ini     # Config PlatformIO
```

## Note

- Questo variant è per **firmware Meshtastic ufficiale**
- Compatibile con tutti i dispositivi Meshtastic
- Usa l'app Meshtastic per messaggiare
- Supporta mesh routing, encryption, GPS (se aggiunto)

## Link Utili

- [Meshtastic Firmware](https://github.com/meshtastic/firmware)
- [Meshtastic Docs](https://meshtastic.org/docs/)
- [PlatformIO](https://platformio.org/)
- [App Android](https://play.google.com/store/apps/details?id=com.geeksville.mesh)
- [App iOS](https://apps.apple.com/app/meshtastic/id1586432531)
