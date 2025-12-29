/**
 * Bit-banged SPI implementation via MCP23017 GPIO expander
 * For connecting SX1262 LoRa module to M5Stack Cardputer
 */

#ifndef MCP23017_SPI_H
#define MCP23017_SPI_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include "config.h"

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
     * Get the MCP23017 instance for direct access if needed
     */
    Adafruit_MCP23X17& getMCP() { return _mcp; }

private:
    Adafruit_MCP23X17 _mcp;
    bool _initialized;

    // Cached port values for faster access
    uint8_t _portA_output;

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
