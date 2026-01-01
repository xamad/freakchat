# Meshtastic Variant: DIY ESP32-S3 + DX-LR-30

Variant Meshtastic personalizzato per ESP32-S3 WROOM con modulo LoRa DX-LR-30 (SX1262) e display SPI 2".

**Versione Firmware:** 2.5.15 (stable)

## Hardware Supportato

- **MCU:** ESP32-S3 WROOM (o DevKitC-1)
- **LoRa:** DX-LR-30 (SX1262, 868MHz)
- **Display:** ST7789 2" SPI (320x240)
- **Tastiera:** App Meshtastic via Bluetooth

## Flash Rapido (Firmware Pre-Compilato)

Il firmware pre-compilato è disponibile nella cartella `diy-esp32s3-dxlr30/firmware/`.

### Usando esptool (consigliato per prima installazione)

```bash
# Collega ESP32-S3 via USB
# Tieni premuto BOOT e premi RESET per entrare in bootloader mode

# Flash completo (prima installazione)
esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 921600 \
    write_flash -z 0x0 firmware/firmware.factory.bin

# Oppure flash update (aggiornamento)
esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 921600 \
    write_flash -z 0x10000 firmware/firmware.bin
```

### Usando PlatformIO (se hai già compilato)

```bash
pio run -e diy-esp32s3-dxlr30 -t upload
```

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
| GPIO37 | MOSI (SDA) | Data |
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

## Compilazione Manuale

### 1. Clona il firmware Meshtastic

```bash
git clone --recursive https://github.com/meshtastic/firmware.git
cd firmware
git checkout v2.5.15.79da236  # Versione stabile
git submodule update --init --recursive
```

### 2. Copia il variant

```bash
# Copia la cartella variant
cp -r /path/to/freakchat/meshtastic-variant/diy-esp32s3-dxlr30 variants/
```

### 3. Compila

```bash
# Installa PlatformIO se non l'hai
pip install platformio

# Compila il firmware
pio run -e diy-esp32s3-dxlr30

# Il firmware sarà in:
# .pio/build/diy-esp32s3-dxlr30/firmware.bin
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
- Controlla pin DC e RST
- Prova a invertire i colori in variant.h

### Bluetooth non trova il dispositivo
- Riavvia il dispositivo
- Disabilita e riabilita Bluetooth sul telefono
- Prova "Forget device" e rifare pairing

## File Inclusi

```
meshtastic-variant/
├── README.md                    # Questa guida
└── diy-esp32s3-dxlr30/
    ├── variant.h                # Definizioni hardware
    ├── pins_arduino.h           # Pin Arduino
    ├── platformio.ini           # Config PlatformIO (da copiare in variants/)
    └── firmware/
        ├── firmware.bin         # Firmware v2.5.15 (update)
        ├── firmware.factory.bin # Firmware v2.5.15 (full flash)
        ├── bootloader.bin       # Bootloader
        └── partitions.bin       # Partizioni
```

## Note

- Firmware basato su **Meshtastic v2.5.15** (stable)
- Compatibile con tutti i dispositivi Meshtastic
- Usa l'app Meshtastic per messaggiare
- Supporta mesh routing, encryption, GPS (se aggiunto)

## Link Utili

- [Meshtastic Firmware](https://github.com/meshtastic/firmware)
- [Meshtastic Docs](https://meshtastic.org/docs/)
- [PlatformIO](https://platformio.org/)
- [App Android](https://play.google.com/store/apps/details?id=com.geeksville.mesh)
- [App iOS](https://apps.apple.com/app/meshtastic/id1586432531)
