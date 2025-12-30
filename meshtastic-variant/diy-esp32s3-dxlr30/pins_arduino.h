/**
 * Arduino Pin Definitions for DIY ESP32-S3 WROOM + DX-LR-30
 */

#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>

// ==================== Default I2C ====================
static const uint8_t SDA = 8;
static const uint8_t SCL = 9;

// ==================== Default SPI (HSPI - used for LoRa) ====================
static const uint8_t SS   = 10;
static const uint8_t MOSI = 11;
static const uint8_t MISO = 13;
static const uint8_t SCK  = 12;

// ==================== VSPI (used for Display) ====================
static const uint8_t SS1   = 36;
static const uint8_t MOSI1 = 37;
static const uint8_t MISO1 = -1;  // Not used for display
static const uint8_t SCK1  = 35;

// ==================== Serial ====================
static const uint8_t TX = 43;
static const uint8_t RX = 44;

// ==================== LED ====================
static const uint8_t LED_BUILTIN = 48;
#define BUILTIN_LED LED_BUILTIN
#define LED_BUILTIN LED_BUILTIN

// ==================== Touch Pins ====================
static const uint8_t T1 = 1;
static const uint8_t T2 = 2;
static const uint8_t T3 = 3;
static const uint8_t T4 = 4;
static const uint8_t T5 = 5;
static const uint8_t T6 = 6;
static const uint8_t T7 = 7;
static const uint8_t T8 = 8;
static const uint8_t T9 = 9;
static const uint8_t T10 = 10;
static const uint8_t T11 = 11;
static const uint8_t T12 = 12;
static const uint8_t T13 = 13;
static const uint8_t T14 = 14;

// ==================== ADC Pins ====================
static const uint8_t A0 = 1;
static const uint8_t A1 = 2;
static const uint8_t A2 = 3;
static const uint8_t A3 = 4;
static const uint8_t A4 = 5;
static const uint8_t A5 = 6;
static const uint8_t A6 = 7;
static const uint8_t A7 = 8;
static const uint8_t A8 = 9;
static const uint8_t A9 = 10;

// ==================== USB ====================
static const uint8_t USB_DN = 19;
static const uint8_t USB_DP = 20;

#endif /* Pins_Arduino_h */
