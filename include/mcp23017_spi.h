/**
 * Bit-banged SPI implementation via MCP23017 GPIO expander
 * For connecting SX1262 LoRa module to M5Stack Cardputer
 *
 * Uses direct I2C register access (no external library)
 */

#ifndef MCP23017_SPI_H
#define MCP23017_SPI_H

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

// MCP23017 Register addresses (IOCON.BANK = 0)
#define MCP_IODIRA   0x00  // I/O direction register A
#define MCP_IODIRB   0x01  // I/O direction register B
#define MCP_IPOLA    0x02  // Input polarity A
#define MCP_IPOLB    0x03  // Input polarity B
#define MCP_GPINTENA 0x04  // Interrupt-on-change A
#define MCP_GPINTENB 0x05  // Interrupt-on-change B
#define MCP_DEFVALA  0x06  // Default compare A
#define MCP_DEFVALB  0x07  // Default compare B
#define MCP_INTCONA  0x08  // Interrupt control A
#define MCP_INTCONB  0x09  // Interrupt control B
#define MCP_IOCON    0x0A  // Configuration
#define MCP_GPPUA    0x0C  // Pull-up resistors A
#define MCP_GPPUB    0x0D  // Pull-up resistors B
#define MCP_INTFA    0x0E  // Interrupt flag A
#define MCP_INTFB    0x0F  // Interrupt flag B
#define MCP_INTCAPA  0x10  // Interrupt capture A
#define MCP_INTCAPB  0x11  // Interrupt capture B
#define MCP_GPIOA    0x12  // Port A
#define MCP_GPIOB    0x13  // Port B
#define MCP_OLATA    0x14  // Output latch A
#define MCP_OLATB    0x15  // Output latch B

class MCP23017_SPI {
public:
    MCP23017_SPI();

    /**
     * Initialize the MCP23017 and configure pins
     * @return true if successful
     */
    bool begin();

    /**
     * Transfer a single byte over SPI (full duplex)
     * @param data Byte to send
     * @return Byte received
     */
    uint8_t transfer(uint8_t data);

    /**
     * Transfer multiple bytes
     * @param buffer Data buffer (will be overwritten with received data)
     * @param length Number of bytes to transfer
     */
    void transfer(uint8_t* buffer, size_t length);

    /**
     * Write multiple bytes (ignore received data)
     * @param buffer Data to send
     * @param length Number of bytes to send
     */
    void write(const uint8_t* buffer, size_t length);

    /**
     * Read multiple bytes (send zeros)
     * @param buffer Buffer to store received data
     * @param length Number of bytes to read
     */
    void read(uint8_t* buffer, size_t length);

    /**
     * Select the SPI device (CS low)
     */
    void select();

    /**
     * Deselect the SPI device (CS high)
     */
    void deselect();

    /**
     * Set the reset pin state
     * @param state HIGH or LOW
     */
    void setReset(bool state);

    /**
     * Read the BUSY pin state
     * @return true if BUSY is high
     */
    bool isBusy();

    /**
     * Wait for BUSY pin to go low
     * @param timeout_ms Maximum wait time in milliseconds
     * @return true if BUSY went low, false on timeout
     */
    bool waitBusy(uint32_t timeout_ms = BUSY_TIMEOUT_MS);

    /**
     * Read the DIO1 interrupt pin state
     * @return true if DIO1 is high
     */
    bool readDIO1();

    /**
     * Get the I2C address found
     */
    uint8_t getAddress() { return _i2cAddr; }

private:
    bool _initialized;
    uint8_t _i2cAddr;

    // Cached port values for faster access
    uint8_t _portA_output;

    /**
     * Write a register on the MCP23017
     */
    bool writeRegister(uint8_t reg, uint8_t value);

    /**
     * Read a register from the MCP23017
     */
    uint8_t readRegister(uint8_t reg);

    /**
     * Write a single bit on the SPI bus (clock pulse)
     * @param bit Bit value to write (0 or 1)
     * @return Bit read from MISO
     */
    uint8_t transferBit(uint8_t bit);

    /**
     * Update PORTA outputs efficiently
     */
    void updateOutputs();

    /**
     * Read PORTA inputs
     * @return PORTA input values
     */
    uint8_t readInputs();
};

// Global instance
extern MCP23017_SPI mcpSPI;

#endif // MCP23017_SPI_H
