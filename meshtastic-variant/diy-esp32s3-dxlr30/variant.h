/**
 * Meshtastic Variant: DIY ESP32-S3 WROOM + DX-LR-30 (SX1262)
 *
 * Custom variant for:
 * - ESP32-S3 WROOM module
 * - DX-LR-30 LoRa module (SX1262)
 * - 2" SPI Display (ST7789 320x240)
 *
 * Pin Mapping:
 * LoRa (HSPI):
 *   GPIO10 -> NSS, GPIO12 -> SCK, GPIO13 -> MISO, GPIO11 -> MOSI
 *   GPIO2 -> DIO1, GPIO3 -> RST, GPIO4 -> BUSY
 *
 * Display (VSPI):
 *   GPIO35 -> SCK, GPIO37 -> MOSI, GPIO36 -> CS
 *   GPIO38 -> DC, GPIO39 -> RST, GPIO40 -> BL
 */

#ifndef _VARIANT_DIY_ESP32S3_DXLR30_
#define _VARIANT_DIY_ESP32S3_DXLR30_

// ==================== Hardware Features ====================
#define HAS_RADIO 1
#define HAS_SCREEN 1
#define HAS_BLUETOOTH 1
#define HAS_WIFI 1

// ==================== LoRa Radio (SX1262) ====================
#define USE_SX1262
#define SX126X_CS       10      // NSS
#define SX126X_DIO1     2       // DIO1 (IRQ)
#define SX126X_BUSY     4       // BUSY
#define SX126X_RESET    3       // RST

// SX1262 SPI Bus (HSPI)
#define LORA_SCK        12
#define LORA_MISO       13
#define LORA_MOSI       11

// DX-LR-30 uses DIO2 for RF switch control
#define SX126X_DIO2_AS_RF_SWITCH

// TCXO voltage (if module has TCXO, DX-LR-30 may not need this)
// #define SX126X_DIO3_TCXO_VOLTAGE 1.8

// ==================== Display (ST7789 320x240) ====================
#define USE_ST7789
#define ST7789_CS       36
#define ST7789_RS       38      // DC
#define ST7789_SDA      37      // MOSI
#define ST7789_SCK      35
#define ST7789_RESET    39
#define ST7789_BL       40      // Backlight

// Display SPI Bus (VSPI)
#define TFT_CS          ST7789_CS
#define TFT_DC          ST7789_RS
#define TFT_RST         ST7789_RESET
#define TFT_BL          ST7789_BL
#define TFT_BACKLIGHT_ON HIGH

// Display dimensions (landscape mode)
#define TFT_HEIGHT      240
#define TFT_WIDTH       320
#define TFT_OFFSET_X    0
#define TFT_OFFSET_Y    0

// Display rotation (1 = landscape, USB on left)
#define SCREEN_ROTATE   true
#define SCREEN_TRANSITION_FRAMERATE 5

// ==================== I2C (for optional sensors) ====================
// Using default I2C pins
#define I2C_SDA         8
#define I2C_SCL         9

// ==================== GPS (optional) ====================
// Uncomment if using GPS module on UART
// #define GPS_RX_PIN      44
// #define GPS_TX_PIN      43
// #define HAS_GPS         1

// ==================== LED ====================
// Built-in LED (if available on your ESP32-S3 board)
#define LED_PIN         48      // RGB LED on some ESP32-S3 boards
// #define LED_INVERTED    0

// ==================== Button ====================
// Boot button can be used as user button
#define BUTTON_PIN      0
#define BUTTON_NEED_PULLUP

// ==================== Power Management ====================
// Battery ADC (if using battery)
// #define BATTERY_PIN     1
// #define ADC_MULTIPLIER  2.0
// #define BATTERY_SENSE_SAMPLES 30

// ==================== USB ====================
#define USB_SERIAL_JTAG

// ==================== Bluetooth ====================
// ESP32-S3 uses NimBLE
#define ARCH_ESP32
#define HAS_BLE_CLASSIC 1

// ==================== SPI Configuration ====================
// We use two SPI buses: HSPI for LoRa, VSPI for Display
#define LORA_SPI_HOST   SPI2_HOST
#define TFT_SPI_HOST    SPI3_HOST

// LoRa SPI settings
#define LORA_SPI_FREQ   10000000    // 10 MHz for LoRa

// Display SPI settings
#define TFT_SPI_FREQ    40000000    // 40 MHz for display

#endif // _VARIANT_DIY_ESP32S3_DXLR30_
