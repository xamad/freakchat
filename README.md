# FreakChat - LoRa Chat for M5Stack Cardputer

Chat application using **MCP23017 GPIO expander** for connecting **DX-LR-30** (SX1262) LoRa module to M5Stack Cardputer via bit-banged SPI over I2C.

## Hardware Required

- **M5Stack Cardputer** (ESP32-S3)
- **Waveshare MCP23017 GPIO Expander** (I2C address 0x27 with A0=A1=A2=HIGH)
- **DX-LR-30 LoRa Module** (SX1262-based, 868MHz)
- Grove-to-Jumper cable
- Antenna for 868MHz

## Wiring Diagram

### Cardputer Grove Port -> MCP23017 (I2C)

| Cardputer Grove | MCP23017 | Description |
|-----------------|----------|-------------|
| G2              | SDA      | I2C Data    |
| G1              | SCL      | I2C Clock   |
| 3.3V            | VCC      | Power       |
| GND             | GND      | Ground      |

**Note:** MCP23017 address is 0x27 (A0=A1=A2 connected to VCC)

### MCP23017 Port A -> DX-LR-30 (SX1262)

| MCP23017 | DX-LR-30    | Function         | Direction |
|----------|-------------|------------------|-----------|
| GPA0     | NSS (Pin 9) | SPI Chip Select  | OUTPUT    |
| GPA1     | MISO (Pin 11)| SPI Data from LoRa | INPUT  |
| GPA2     | SCK (Pin 10)| SPI Clock        | OUTPUT    |
| GPA3     | RST (Pin 17)| Reset            | OUTPUT    |
| GPA4     | DIO1 (Pin 13)| TX/RX Done IRQ  | INPUT     |
| GPA5     | BUSY (Pin 21)| Busy Indicator  | INPUT     |
| GPA6     | MOSI (Pin 12)| SPI Data to LoRa | OUTPUT   |

### DX-LR-30 Power Connections

| DX-LR-30 | Connection | Description |
|----------|------------|-------------|
| VCC (Pin 1) | 3.3V    | Power (3.3V only!) |
| GND (Pin 2) | GND     | Ground      |
| ANT (Pin 7) | Antenna | 868MHz antenna |

## Pin Summary (config.h)

```c
// I2C Configuration
#define I2C_SDA         2       // Cardputer Grove G2
#define I2C_SCL         1       // Cardputer Grove G1
#define MCP23017_ADDR   0x27    // A0=A1=A2=HIGH

// MCP23017 Port A Pin Mapping
#define MCP_LORA_NSS    0       // GPA0 - Chip Select (OUTPUT)
#define MCP_LORA_MISO   1       // GPA1 - SPI Data from LoRa (INPUT)
#define MCP_LORA_SCK    2       // GPA2 - SPI Clock (OUTPUT)
#define MCP_LORA_RST    3       // GPA3 - Reset (OUTPUT)
#define MCP_LORA_DIO1   4       // GPA4 - TX/RX Interrupt (INPUT)
#define MCP_LORA_BUSY   5       // GPA5 - Busy indicator (INPUT)
#define MCP_LORA_MOSI   6       // GPA6 - SPI Data to LoRa (OUTPUT)
```

## LoRa Configuration

```c
#define LORA_FREQUENCY      868.0   // MHz (Europe: 868, USA: 915, Asia: 433)
#define LORA_BANDWIDTH      125.0   // kHz
#define LORA_SPREADING      9       // SF7-SF12
#define LORA_CODING_RATE    7       // 4/5 to 4/8
#define LORA_SYNC_WORD      0x12    // Private network
#define LORA_TX_POWER       14      // dBm (max 22)
#define LORA_PREAMBLE_LEN   8       // Symbols
```

## Building

```bash
# Install PlatformIO
pip install platformio

# Build firmware
pio run

# Upload to Cardputer (connected via USB)
pio run -t upload

# Monitor serial output (optional debug)
pio device monitor
```

## Flashing Pre-built Firmware

```bash
esptool.py --chip esp32s3 --port /dev/ttyUSB0 write_flash 0x10000 firmware.bin
```

Or use [M5Burner](https://m5stack.com/pages/download) application.

## Usage

1. **Power on** - Radio initialization shows on screen (all steps should show OK)
2. **Enter nickname** - Type your name and press ENTER
3. **Chat** - Type messages and press ENTER to send

### Commands

| Command | Description |
|---------|-------------|
| `/ping` | Test TX - shows visual feedback (green=OK, red=fail) |
| `/nick <name>` | Change nickname |
| `/freq` | Show current frequency |
| `/help` | Show available commands |

### GPIO Test Mode

Hold any key during boot to enter GPIO test mode for debugging wiring:
- Shows real-time state of all MCP23017 pins
- **R** - Toggle RST pin
- **N** - Toggle NSS pin
- **S** - Toggle SCK pin
- **M** - Toggle MOSI pin
- **T** - Send reset pulse and watch BUSY
- **Q** - Quit and continue to chat

## Technical Notes

### Bit-banged SPI over I2C

The SX1262 is connected via bit-banged SPI through the MCP23017:
- Each SPI byte transfer requires 16+ I2C transactions
- Effective SPI clock speed: ~10-50 kHz (very slow compared to hardware SPI)
- I2C bus speed: 100 kHz (set low for reliability)

### TX Timing

Due to slow bit-banged SPI, TX uses a **time-based approach** instead of polling:
- After sending SetTX command, wait 500ms fixed delay
- Then check BUSY/DIO1/IRQ flags to confirm completion
- This is necessary because polling would be too slow to catch state changes

For SF9, BW125:
- Symbol time: ~4ms
- Preamble (8 symbols): ~50ms
- Short message (~10 bytes): ~200-300ms total air time

## Troubleshooting

### "MCP23017 NOT FOUND"
- Check I2C wiring: SDA -> G2, SCL -> G1
- Verify MCP23017 address jumpers (A0/A1/A2 all HIGH = 0x27)
- Check power connections (3.3V and GND)
- The firmware auto-scans addresses 0x20-0x27

### "Reset BUSY stuck"
- Check RST wiring: GPA3 -> RST (Pin 17)
- Check BUSY wiring: GPA5 -> BUSY (Pin 21)
- Verify DX-LR-30 has proper 3.3V power
- Use GPIO test mode (hold key at boot) to manually test pins

### "SPI FAIL (0xFF 0xFF)"
- Check SPI wiring:
  - NSS: GPA0 -> Pin 9
  - SCK: GPA2 -> Pin 10
  - MOSI: GPA6 -> Pin 12
  - MISO: GPA1 -> Pin 11
- Verify all ground connections are solid

### "TX Failed"
- Make sure antenna is connected
- Check DIO1 wiring: GPA4 -> DIO1 (Pin 13)
- Use `/ping` command for visual TX test
- Radio should show "RADIO OK" at startup

## Features

- LoRa communication on configurable frequency
- Simple chat protocol with nickname
- RSSI signal strength indicator on received messages
- Nickname persistence (saved to flash)
- Visual TX feedback
- GPIO test mode for debugging

## License

MIT License

## Credits

- M5Stack for the Cardputer hardware
- Semtech for SX1262 LoRa chip
- Waveshare for MCP23017 module
