/**
 * TFT_eSPI User Setup for MeshtasticDIY
 *
 * IMPORTANT: Copy this file to the TFT_eSPI library folder!
 * Path: Arduino/libraries/TFT_eSPI/User_Setup.h
 *
 * Display: 2" SPI (ST7789 or ILI9341 240x320)
 * MCU: ESP32-S3 WROOM
 */

// ==================== Display Driver ====================
// Uncomment ONE driver for your display

#define ST7789_DRIVER      // 2" 240x320 (most common)
// #define ILI9341_DRIVER   // Alternative 2" driver

// ==================== Display Size ====================
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// ==================== ESP32-S3 SPI Pins ====================
// Using VSPI (SPI3) for display, HSPI (SPI2) is used for LoRa

#define TFT_MOSI 37
#define TFT_SCLK 35
#define TFT_CS   36
#define TFT_DC   38
#define TFT_RST  39
#define TFT_BL   40  // Backlight (optional, -1 if not used)

// MISO not needed for display (write only)
#define TFT_MISO -1

// ==================== SPI Settings ====================
#define SPI_FREQUENCY  40000000   // 40 MHz
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000

// Use hardware SPI
#define USE_HSPI_PORT  // Use HSPI for display (ESP32-S3)

// ==================== Fonts ====================
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font
#define LOAD_FONT2  // Font 2. Small 16 pixel high font
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font
#define LOAD_FONT6  // Font 6. Large 48 pixel font
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font
#define LOAD_FONT8  // Font 8. Large 75 pixel font
#define LOAD_GFXFF  // FreeFonts

#define SMOOTH_FONT

// ==================== Color Order ====================
// Uncomment if colors are inverted
// #define TFT_INVERSION_ON
// #define TFT_RGB_ORDER TFT_BGR
