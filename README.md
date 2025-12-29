# LoRa Chat for M5Stack Cardputer

Chat application using **MCP23017 GPIO expander** for connecting **DX-LR-30** (SX1262) LoRa module to M5Stack Cardputer via bit-banged SPI over I2C.

## Hardware Requirements

- M5Stack Cardputer
- Waveshare MCP23017 GPIO Expander
- DX-LR-30 LoRa Module (SX1262-based)
- Grove-to-Jumper cable
- Jumper wires

## Wiring Diagram

### Cardputer to MCP23017 (I2C via Grove)

| Cardputer Grove | MCP23017 |
|-----------------|----------|
| G1 (SCL)        | SCL      |
| G2 (SDA)        | SDA      |
| 3.3V            | VCC      |
| GND             | GND      |

### MCP23017 to DX-LR-30 (SPI via GPIO)

| MCP23017 Pin | DX-LR-30 Pin | Function    |
|--------------|--------------|-------------|
| GPA0         | Pin 9 (NSS)  | Chip Select |
| GPA1         | Pin 10 (SCK) | SPI Clock   |
| GPA2         | Pin 11 (MISO)| SPI Data Out|
| GPA3         | Pin 12 (MOSI)| SPI Data In |
| GPA4         | Pin 17 (RST) | Reset       |
| GPA5         | Pin 21 (BUSY)| Busy Status |

### Power

- DX-LR-30 VCC (Pin 29) -> 3.3V from Cardputer
- DX-LR-30 GND (Pin 18) -> Common GND

## Configuration

Edit `include/config.h` to customize:

```cpp
// I2C pins (Cardputer Grove)
#define I2C_SDA         2
#define I2C_SCL         1

// MCP23017 address (A0, A1, A2 grounded = 0x20)
#define MCP23017_ADDR   0x20

// LoRa frequency (MHz)
#define LORA_FREQUENCY  868.0   // Europe
// #define LORA_FREQUENCY  915.0   // USA
// #define LORA_FREQUENCY  433.0   // Asia
```

## Building

### Prerequisites

- PlatformIO CLI or IDE

### Build Commands

```bash
# Build firmware
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor
```

## Flashing Pre-built Firmware

Use esptool.py to flash the pre-built `firmware.bin`:

```bash
esptool.py --chip esp32s3 --port /dev/ttyUSB0 write_flash 0x10000 firmware.bin
```

Or use the [M5Burner](https://m5stack.com/pages/download) application.

## Usage

1. Power on the Cardputer
2. Enter your nickname at startup
3. Type messages using the keyboard
4. Press ENTER to send
5. Received messages show sender nickname and signal strength

### Commands

- `/nick <name>` - Change nickname
- `/freq` - Show current frequency
- `/help` - Show available commands

## Features

- 868 MHz LoRa communication (configurable)
- Simple chat protocol
- RSSI signal strength indicator
- Nickname persistence (saved to flash)
- Bit-banged SPI for flexibility

## Technical Details

- **SPI Mode**: Bit-banged through MCP23017 GPIO
- **I2C Speed**: 400 kHz
- **LoRa Settings**: SF9, BW125kHz, CR4/7
- **Max Message Length**: 200 characters
- **Display**: 240x135 pixels

## License

MIT License

## Credits

- M5Stack for the Cardputer
- Semtech for SX1262 chip
- Waveshare for MCP23017 module
