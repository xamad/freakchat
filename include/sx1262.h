/**
 * SX1262 LoRa Radio Driver using MCP23017 bit-banged SPI
 * For DX-LR-30 module
 */

#ifndef SX1262_H
#define SX1262_H

#include <Arduino.h>
#include "mcp23017_spi.h"
#include "config.h"

// SX1262 Commands
#define SX1262_CMD_SET_SLEEP                0x84
#define SX1262_CMD_SET_STANDBY              0x80
#define SX1262_CMD_SET_FS                   0xC1
#define SX1262_CMD_SET_TX                   0x83
#define SX1262_CMD_SET_RX                   0x82
#define SX1262_CMD_STOP_TIMER_ON_PREAMBLE   0x9F
#define SX1262_CMD_SET_RX_DUTY_CYCLE        0x94
#define SX1262_CMD_SET_CAD                  0xC5
#define SX1262_CMD_SET_TX_CONTINUOUS_WAVE   0xD1
#define SX1262_CMD_SET_TX_INFINITE_PREAMBLE 0xD2
#define SX1262_CMD_SET_REGULATOR_MODE       0x96
#define SX1262_CMD_CALIBRATE                0x89
#define SX1262_CMD_CALIBRATE_IMAGE          0x98
#define SX1262_CMD_SET_PA_CONFIG            0x95
#define SX1262_CMD_SET_RX_TX_FALLBACK_MODE  0x93

#define SX1262_CMD_WRITE_REGISTER           0x0D
#define SX1262_CMD_READ_REGISTER            0x1D
#define SX1262_CMD_WRITE_BUFFER             0x0E
#define SX1262_CMD_READ_BUFFER              0x1E

#define SX1262_CMD_SET_DIO_IRQ_PARAMS       0x08
#define SX1262_CMD_GET_IRQ_STATUS           0x12
#define SX1262_CMD_CLEAR_IRQ_STATUS         0x02
#define SX1262_CMD_SET_DIO2_AS_RF_SW_CTRL   0x9D
#define SX1262_CMD_SET_DIO3_AS_TCXO_CTRL    0x97

#define SX1262_CMD_SET_RF_FREQUENCY         0x86
#define SX1262_CMD_SET_PACKET_TYPE          0x8A
#define SX1262_CMD_GET_PACKET_TYPE          0x11
#define SX1262_CMD_SET_TX_PARAMS            0x8E
#define SX1262_CMD_SET_MODULATION_PARAMS    0x8B
#define SX1262_CMD_SET_PACKET_PARAMS        0x8C
#define SX1262_CMD_SET_CAD_PARAMS           0x88
#define SX1262_CMD_SET_BUFFER_BASE_ADDRESS  0x8F
#define SX1262_CMD_SET_LORA_SYMB_NUM_TIMEOUT 0xA0

#define SX1262_CMD_GET_STATUS               0xC0
#define SX1262_CMD_GET_RX_BUFFER_STATUS     0x13
#define SX1262_CMD_GET_PACKET_STATUS        0x14
#define SX1262_CMD_GET_RSSI_INST            0x15
#define SX1262_CMD_GET_STATS                0x10
#define SX1262_CMD_RESET_STATS              0x00
#define SX1262_CMD_GET_DEVICE_ERRORS        0x17
#define SX1262_CMD_CLEAR_DEVICE_ERRORS      0x07

// Standby modes
#define SX1262_STANDBY_RC                   0x00
#define SX1262_STANDBY_XOSC                 0x01

// Regulator modes
#define SX1262_REGULATOR_LDO                0x00
#define SX1262_REGULATOR_DC_DC              0x01

// Packet types
#define SX1262_PACKET_TYPE_GFSK             0x00
#define SX1262_PACKET_TYPE_LORA             0x01

// IRQ flags
#define SX1262_IRQ_TX_DONE                  0x0001
#define SX1262_IRQ_RX_DONE                  0x0002
#define SX1262_IRQ_PREAMBLE_DETECTED        0x0004
#define SX1262_IRQ_SYNC_WORD_VALID          0x0008
#define SX1262_IRQ_HEADER_VALID             0x0010
#define SX1262_IRQ_HEADER_ERR               0x0020
#define SX1262_IRQ_CRC_ERR                  0x0040
#define SX1262_IRQ_CAD_DONE                 0x0080
#define SX1262_IRQ_CAD_DETECTED             0x0100
#define SX1262_IRQ_TIMEOUT                  0x0200
#define SX1262_IRQ_ALL                      0x03FF

// Spreading factors
#define SX1262_SF5                          0x05
#define SX1262_SF6                          0x06
#define SX1262_SF7                          0x07
#define SX1262_SF8                          0x08
#define SX1262_SF9                          0x09
#define SX1262_SF10                         0x0A
#define SX1262_SF11                         0x0B
#define SX1262_SF12                         0x0C

// Bandwidths
#define SX1262_BW_7_8                       0x00
#define SX1262_BW_10_4                      0x08
#define SX1262_BW_15_6                      0x01
#define SX1262_BW_20_8                      0x09
#define SX1262_BW_31_25                     0x02
#define SX1262_BW_41_7                      0x0A
#define SX1262_BW_62_5                      0x03
#define SX1262_BW_125                       0x04
#define SX1262_BW_250                       0x05
#define SX1262_BW_500                       0x06

// Coding rates
#define SX1262_CR_4_5                       0x01
#define SX1262_CR_4_6                       0x02
#define SX1262_CR_4_7                       0x03
#define SX1262_CR_4_8                       0x04

class SX1262 {
public:
    SX1262();

    /**
     * Initialize the radio
     * @return true if successful
     */
    bool begin();

    /**
     * Reset the radio module
     */
    void reset();

    /**
     * Configure LoRa parameters
     * @param freq Frequency in MHz
     * @param bw Bandwidth in kHz
     * @param sf Spreading factor (5-12)
     * @param cr Coding rate (5-8)
     * @return true if successful
     */
    bool configure(float freq, float bw, uint8_t sf, uint8_t cr);

    /**
     * Set the sync word
     * @param syncWord Sync word (0x12 for private, 0x34 for public/LoRaWAN)
     */
    void setSyncWord(uint8_t syncWord);

    /**
     * Set transmit power
     * @param power Power in dBm (-9 to 22)
     */
    void setTxPower(int8_t power);

    /**
     * Transmit a packet
     * @param data Data buffer
     * @param length Data length
     * @param timeout_ms Timeout in milliseconds
     * @return true if successful
     */
    bool transmit(const uint8_t* data, size_t length, uint32_t timeout_ms = TX_TIMEOUT_MS);

    /**
     * Start receiving
     * @param timeout_ms Receive timeout (0 = single mode, 0xFFFFFF = continuous)
     */
    void startReceive(uint32_t timeout_ms = 0xFFFFFF);

    /**
     * Check if a packet is available
     * @return true if packet received
     */
    bool available();

    /**
     * Read received packet
     * @param buffer Buffer to store data
     * @param maxLength Maximum buffer size
     * @return Number of bytes received
     */
    size_t readPacket(uint8_t* buffer, size_t maxLength);

    /**
     * Get RSSI of last received packet
     * @return RSSI in dBm
     */
    int16_t getPacketRSSI();

    /**
     * Get SNR of last received packet
     * @return SNR in dB
     */
    int8_t getPacketSNR();

    /**
     * Set to standby mode
     */
    void standby();

    /**
     * Set to sleep mode
     */
    void sleep();

    /**
     * Check for device errors
     * @return Error code
     */
    uint16_t getDeviceErrors();

    /**
     * Get the current status
     * @return Status byte
     */
    uint8_t getStatus();

private:
    MCP23017_SPI* _spi;
    float _frequency;
    int16_t _lastRSSI;
    int8_t _lastSNR;
    bool _implicitHeader;

    // SPI communication helpers
    void sendCommand(uint8_t cmd);
    void sendCommand(uint8_t cmd, uint8_t* data, size_t length);
    void readCommand(uint8_t cmd, uint8_t* data, size_t length);
    void writeRegister(uint16_t addr, uint8_t value);
    uint8_t readRegister(uint16_t addr);
    void writeBuffer(uint8_t offset, const uint8_t* data, size_t length);
    void readBuffer(uint8_t offset, uint8_t* data, size_t length);

    // Configuration helpers
    void setPacketType(uint8_t type);
    void setFrequency(float freq);
    void setModulationParams(uint8_t sf, uint8_t bw, uint8_t cr, uint8_t ldro);
    void setPacketParams(uint16_t preambleLen, bool implicitHeader, uint8_t payloadLen, bool crcOn, bool invertIQ);
    void setDio2AsRfSwitch(bool enable);
    void setDioIrqParams(uint16_t irqMask, uint16_t dio1Mask, uint16_t dio2Mask, uint16_t dio3Mask);
    uint16_t getIrqStatus();
    void clearIrqStatus(uint16_t mask);
    void setBufferBaseAddress(uint8_t txBase, uint8_t rxBase);
    void setRegulatorMode(uint8_t mode);
    void calibrateImage(float freq);

    // Convert bandwidth to register value
    uint8_t bandwidthToReg(float bw);
};

// Global instance
extern SX1262 radio;

#endif // SX1262_H
