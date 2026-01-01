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

// ==================== Button ====================
#define BUTTON_PIN 0
#define BUTTON_NEED_PULLUP

// ==================== I2C ====================
#define I2C_SDA 8
#define I2C_SCL 9

// ==================== Display (ST7789 320x240) ====================
#define USE_ST7789
#define ST7789_NSS 36       // CS
#define ST7789_RS 38        // DC
#define ST7789_SDA 37       // MOSI
#define ST7789_SCK 35
#define ST7789_RESET 39
#define ST7789_MISO -1      // Not used for display
#define ST7789_BUSY -1
#define VTFT_LEDA 40        // Backlight
#define VTFT_CTRL 21        // TFT power control (not connected, placeholder)
#define TFT_BACKLIGHT_ON HIGH
#define ST7789_SPI_HOST SPI3_HOST
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 10000000
#define TFT_HEIGHT 240
#define TFT_WIDTH 320
#define TFT_OFFSET_X 0
#define TFT_OFFSET_Y 0
#define BRIGHTNESS_DEFAULT 100

// ==================== SPI ====================
#define SPI_INTERFACES_COUNT 2
#define PIN_SPI_MISO 13
#define PIN_SPI_MOSI 11
#define PIN_SPI_SCK 12

// ==================== LoRa Radio (SX1262) ====================
#define USE_SX1262

#define LORA_DIO0 RADIOLIB_NC
#define LORA_RESET 3
#define LORA_DIO1 2         // IRQ
#define LORA_DIO2 4         // BUSY

#define LORA_SCK 12
#define LORA_MISO 13
#define LORA_MOSI 11
#define LORA_CS 10

#define SX126X_CS LORA_CS
#define SX126X_DIO1 LORA_DIO1
#define SX126X_BUSY LORA_DIO2
#define SX126X_RESET LORA_RESET

// DX-LR-30 uses DIO2 for RF switch control
#define SX126X_DIO2_AS_RF_SWITCH

// ==================== GPS (optional) ====================
#define HAS_GPS 0

// ==================== LED ====================
// #define LED_PIN 48

#endif // _VARIANT_DIY_ESP32S3_DXLR30_
