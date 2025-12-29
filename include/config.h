/**
 * Configuration for Cardputer LoRa Chat
 * Using MCP23017 GPIO expander for SX1262 (DX-LR-30)
 */

#ifndef CONFIG_H
#define CONFIG_H

// ==================== I2C Configuration (Cardputer Grove) ====================
// Cardputer Grove port pinout (Port A)
#ifndef I2C_SDA
#define I2C_SDA         2       // Cardputer Grove SDA (G2)
#endif
#ifndef I2C_SCL
#define I2C_SCL         1       // Cardputer Grove SCL (G1)
#endif

// Alternative: Use Port GPIO (if you connected to different pins)
// Uncomment these if using the GPIO port instead of Grove
// #define I2C_SDA      43
// #define I2C_SCL      46

// MCP23017 I2C address (A0, A1, A2 all grounded = 0x20)
#ifndef MCP23017_ADDR
#define MCP23017_ADDR   0x20
#endif

// ==================== MCP23017 Pin Mapping for SX1262 ====================
// DX-LR-30 connections to MCP23017 Port A (GPA0-GPA7)
#define MCP_LORA_NSS    0       // GPA0 - Chip Select (OUTPUT)
#define MCP_LORA_SCK    1       // GPA1 - SPI Clock (OUTPUT)
#define MCP_LORA_MISO   2       // GPA2 - SPI Data from LoRa (INPUT)
#define MCP_LORA_MOSI   3       // GPA3 - SPI Data to LoRa (OUTPUT)
#define MCP_LORA_RST    4       // GPA4 - Reset (OUTPUT)
#define MCP_LORA_BUSY   5       // GPA5 - Busy indicator (INPUT)
#define MCP_LORA_DIO1   6       // GPA6 - Interrupt (INPUT) - optional

// ==================== LoRa Configuration ====================
#ifndef LORA_FREQUENCY
#define LORA_FREQUENCY  868.0   // MHz (Europe: 868, USA: 915, Asia: 433)
#endif

#define LORA_BANDWIDTH      125.0   // kHz (7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125, 250, 500)
#define LORA_SPREADING      9       // Spreading factor (6-12)
#define LORA_CODING_RATE    7       // Coding rate denominator (5-8)
#define LORA_SYNC_WORD      0x12    // Private network sync word
#define LORA_TX_POWER       14      // dBm (max 22 for SX1262)
#define LORA_PREAMBLE_LEN   8       // Preamble symbols

// ==================== Chat Configuration ====================
#define MAX_MSG_LENGTH      200
#define MAX_NICKNAME_LEN    16
#define MSG_HISTORY_SIZE    20

// ==================== Display Configuration ====================
#define DISPLAY_WIDTH       240
#define DISPLAY_HEIGHT      135
#define FONT_HEIGHT         12
#define MSG_LINES           8

// ==================== Timing Configuration ====================
#define SPI_DELAY_US        1       // Bit-bang SPI delay (microseconds)
#define BUSY_TIMEOUT_MS     1000    // Max wait for BUSY pin
#define TX_TIMEOUT_MS       3000    // Transmission timeout

#endif // CONFIG_H
