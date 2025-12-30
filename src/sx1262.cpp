/**
 * SX1262 LoRa Radio Driver implementation
 */

#include "sx1262.h"
#include <M5Cardputer.h>

// Global instance
SX1262 radio;

SX1262::SX1262()
    : _spi(&mcpSPI)
    , _frequency(LORA_FREQUENCY)
    , _lastRSSI(0)
    , _lastSNR(0)
    , _implicitHeader(false)
{
}

bool SX1262::begin() {
    Serial.println("\n=== SX1262::begin() ===");

    auto& lcd = M5Cardputer.Display;

    // Step 1: Initialize MCP23017
    Serial.println("Step 1: Init MCP23017...");
    if (!_spi->begin()) {
        Serial.println("FAILED: MCP23017 init");
        return false;
    }
    Serial.println("MCP23017 OK");

    // Clear screen for SX1262 init status
    lcd.fillScreen(TFT_BLACK);
    lcd.setTextSize(1);
    lcd.setTextColor(TFT_CYAN);
    lcd.setCursor(5, 5);
    lcd.println("SX1262 Init");
    lcd.setTextColor(TFT_WHITE);

    int line = 0;
    auto showStatus = [&](const char* step, bool ok, const char* extra = nullptr) {
        int y = 20 + line * 12;
        lcd.setCursor(5, y);
        lcd.setTextColor(TFT_WHITE);
        lcd.print(step);
        lcd.setTextColor(ok ? TFT_GREEN : TFT_RED);
        lcd.println(ok ? " OK" : " FAIL");
        if (extra) {
            lcd.setTextColor(TFT_YELLOW);
            lcd.setCursor(10, y + 12);
            lcd.println(extra);
            line++;
        }
        line++;
    };

    // Step 2: Reset SX1262
    Serial.println("Step 2: Reset SX1262...");
    _spi->deselect();
    delay(10);
    _spi->setReset(false);
    delay(50);
    _spi->setReset(true);
    delay(100);

    // Wait for BUSY
    int waitMs = 0;
    while (_spi->isBusy() && waitMs < 500) {
        delay(10);
        waitMs += 10;
    }
    bool busyOK = !_spi->isBusy();
    Serial.printf("Reset: BUSY=%d after %dms\n", busyOK ? 0 : 1, waitMs);
    showStatus("Reset", busyOK, busyOK ? nullptr : "BUSY stuck!");

    if (!busyOK) {
        lcd.setTextColor(TFT_YELLOW);
        lcd.println("Check: RST->PA3, BUSY->PA5");
        lcd.println("Check: VCC=3.3V, GND");
        delay(5000);
        return false;
    }

    // Step 3: SPI test
    Serial.println("Step 3: SPI test...");
    _spi->select();
    uint8_t r1 = _spi->transfer(SX1262_CMD_GET_STATUS);
    uint8_t r2 = _spi->transfer(0x00);
    _spi->deselect();
    Serial.printf("SPI: 0x%02X 0x%02X\n", r1, r2);

    bool spiOK = (r1 != 0xFF && r2 != 0xFF);
    char spiBuf[20];
    snprintf(spiBuf, sizeof(spiBuf), "0x%02X 0x%02X", r1, r2);
    showStatus("SPI", spiOK, spiBuf);

    if (!spiOK) {
        lcd.setTextColor(TFT_YELLOW);
        lcd.println("Check SPI wiring:");
        lcd.println(" NSS->PA0 SCK->PA2");
        lcd.println(" MOSI->PA6 MISO->PA1");
        delay(5000);
        return false;
    }

    // Step 4: Standby
    Serial.println("Step 4: Standby...");
    standby();
    bool stbyOK = _spi->waitBusy(1000);
    Serial.printf("Standby: %s\n", stbyOK ? "OK" : "TIMEOUT");
    showStatus("Standby", stbyOK);
    if (!stbyOK) return false;

    // Step 5: Set regulator (DC-DC)
    Serial.println("Step 5: Regulator...");
    setRegulatorMode(SX1262_REGULATOR_DC_DC);
    bool regOK = _spi->waitBusy(500);
    showStatus("Regulator", regOK);
    if (!regOK) return false;

    // Step 6: Set LoRa packet type
    Serial.println("Step 6: LoRa mode...");
    setPacketType(SX1262_PACKET_TYPE_LORA);
    bool loraOK = _spi->waitBusy(500);
    showStatus("LoRa mode", loraOK);
    if (!loraOK) return false;

    // Step 7: DIO2 as RF switch
    Serial.println("Step 7: DIO2...");
    setDio2AsRfSwitch(true);
    bool dio2OK = _spi->waitBusy(500);
    showStatus("DIO2 RF", dio2OK);
    if (!dio2OK) return false;

    // Step 8: Calibrate
    Serial.println("Step 8: Calibrate...");
    calibrateImage(_frequency);
    bool calOK = _spi->waitBusy(500);
    showStatus("Calibrate", calOK);
    if (!calOK) return false;

    // Step 9: Buffer addresses
    Serial.println("Step 9: Buffer...");
    setBufferBaseAddress(0x00, 0x00);
    bool bufOK = _spi->waitBusy(500);
    showStatus("Buffer", bufOK);
    if (!bufOK) return false;

    // Success!
    lcd.setTextColor(TFT_GREEN);
    lcd.setCursor(5, 20 + line * 12);
    lcd.println("=== RADIO OK ===");

    Serial.println("SX1262 initialized OK!");
    delay(1500);
    return true;
}

void SX1262::reset() {
    // Ensure NSS is high first
    _spi->deselect();
    delay(5);

    // Pull RST low
    _spi->setReset(false);
    delay(50);  // Hold low for 50ms

    // Release RST (high)
    _spi->setReset(true);
    delay(100);  // Wait for chip to wake up

    // Wait for BUSY to go low
    _spi->waitBusy(500);
}

bool SX1262::configure(float freq, float bw, uint8_t sf, uint8_t cr) {
    _frequency = freq;

    // Set frequency
    setFrequency(freq);
    _spi->waitBusy();

    // Calculate LDRO (low data rate optimization)
    // Enable for long symbol times (SF11/12 with BW125)
    uint8_t ldro = 0;
    float symbolTime = (1 << sf) / (bw * 1000.0);
    if (symbolTime > 0.016) {  // > 16ms
        ldro = 1;
    }

    // Set modulation parameters
    setModulationParams(sf, bandwidthToReg(bw), cr - 4, ldro);
    _spi->waitBusy();

    // Set packet parameters (variable length, CRC on)
    setPacketParams(LORA_PREAMBLE_LEN, false, 255, true, false);
    _spi->waitBusy();

    // Configure IRQs
    setDioIrqParams(SX1262_IRQ_ALL, SX1262_IRQ_TX_DONE | SX1262_IRQ_RX_DONE | SX1262_IRQ_TIMEOUT, 0, 0);
    _spi->waitBusy();

    Serial.printf("Configured: %.1f MHz, BW=%.0f kHz, SF=%d, CR=4/%d\n", freq, bw, sf, cr);
    return true;
}

void SX1262::setSyncWord(uint8_t syncWord) {
    // LoRa sync word registers
    uint8_t msb = (syncWord & 0xF0) | ((syncWord & 0x0F) << 4);
    uint8_t lsb = ((syncWord & 0xF0) >> 4) | (syncWord & 0x0F);
    writeRegister(0x0740, msb);
    writeRegister(0x0741, lsb);
    _spi->waitBusy();
}

void SX1262::setTxPower(int8_t power) {
    // Clamp power to valid range
    if (power > 22) power = 22;
    if (power < -9) power = -9;

    // Set PA config for high power
    uint8_t paConfig[4];
    if (power <= 14) {
        paConfig[0] = 0x02;  // paDutyCycle
        paConfig[1] = 0x02;  // hpMax
        paConfig[2] = 0x00;  // deviceSel (SX1262)
        paConfig[3] = 0x01;  // paLut
    } else {
        paConfig[0] = 0x04;  // paDutyCycle
        paConfig[1] = 0x07;  // hpMax
        paConfig[2] = 0x00;  // deviceSel
        paConfig[3] = 0x01;  // paLut
    }
    sendCommand(SX1262_CMD_SET_PA_CONFIG, paConfig, 4);
    _spi->waitBusy();

    // Set TX params
    uint8_t txParams[2];
    txParams[0] = (uint8_t)power;  // Power
    txParams[1] = 0x04;            // Ramp time 200us
    sendCommand(SX1262_CMD_SET_TX_PARAMS, txParams, 2);
    _spi->waitBusy();
}

bool SX1262::transmit(const uint8_t* data, size_t length, uint32_t timeout_ms) {
    Serial.printf("TX: %d bytes\n", length);

    // Standby first
    standby();
    if (!_spi->waitBusy(1000)) {
        Serial.println("TX: Standby timeout");
        return false;
    }

    // Clear IRQ flags
    clearIrqStatus(SX1262_IRQ_ALL);
    _spi->waitBusy();

    // Configure DIO1 for TX_DONE
    setDioIrqParams(SX1262_IRQ_TX_DONE | SX1262_IRQ_TIMEOUT,
                    SX1262_IRQ_TX_DONE | SX1262_IRQ_TIMEOUT, 0, 0);
    _spi->waitBusy();

    // Set packet params
    setPacketParams(LORA_PREAMBLE_LEN, _implicitHeader, length, true, false);
    _spi->waitBusy();

    // Write data to buffer
    writeBuffer(0x00, data, length);
    _spi->waitBusy();

    // Send TX command
    uint8_t txCmd[3] = {0, 0, 0};
    sendCommand(SX1262_CMD_SET_TX, txCmd, 3);

    // Wait fixed time for TX to complete (SF9 BW125 ~300ms for short packet)
    // Using time-based approach because bit-banged SPI is too slow for polling
    delay(500);

    // Check result
    bool busy = _spi->isBusy();
    bool dio1 = _spi->readDIO1();
    uint16_t irq = getIrqStatus();

    Serial.printf("TX: BUSY=%d DIO1=%d IRQ=0x%04X\n", busy, dio1, irq);

    // TX complete if IRQ flag set, DIO1 high, or BUSY low
    if ((irq & SX1262_IRQ_TX_DONE) || dio1 || !busy) {
        clearIrqStatus(SX1262_IRQ_ALL);
        Serial.println("TX: OK");
        return true;
    }

    // Still busy after 500ms - fail
    Serial.println("TX: Timeout");
    standby();
    clearIrqStatus(SX1262_IRQ_ALL);
    return false;
}

void SX1262::startReceive(uint32_t timeout_ms) {
    // Standby first
    standby();
    _spi->waitBusy();

    // Clear IRQ flags
    clearIrqStatus(SX1262_IRQ_ALL);
    _spi->waitBusy();

    // Configure for maximum payload
    setPacketParams(LORA_PREAMBLE_LEN, _implicitHeader, 255, true, false);
    _spi->waitBusy();

    // Convert timeout to 15.625us steps
    uint32_t timeout = 0;
    if (timeout_ms == 0xFFFFFF) {
        timeout = 0xFFFFFF;  // Continuous RX
    } else if (timeout_ms > 0) {
        timeout = (timeout_ms * 1000) / 15625;
        if (timeout > 0xFFFFFF) timeout = 0xFFFFFF;
    }

    // Start RX
    uint8_t rxCmd[3];
    rxCmd[0] = (timeout >> 16) & 0xFF;
    rxCmd[1] = (timeout >> 8) & 0xFF;
    rxCmd[2] = timeout & 0xFF;
    sendCommand(SX1262_CMD_SET_RX, rxCmd, 3);
    _spi->waitBusy();
}

bool SX1262::available() {
    uint16_t irq = getIrqStatus();
    return (irq & SX1262_IRQ_RX_DONE) != 0;
}

size_t SX1262::readPacket(uint8_t* buffer, size_t maxLength) {
    // Clear RX done flag
    clearIrqStatus(SX1262_IRQ_RX_DONE);

    // Get RX buffer status
    uint8_t rxStatus[3];
    readCommand(SX1262_CMD_GET_RX_BUFFER_STATUS, rxStatus, 3);
    uint8_t payloadLen = rxStatus[1];
    uint8_t startAddr = rxStatus[2];

    if (payloadLen > maxLength) {
        payloadLen = maxLength;
    }

    // Read packet data
    readBuffer(startAddr, buffer, payloadLen);

    // Get packet status (RSSI, SNR)
    uint8_t pktStatus[4];
    readCommand(SX1262_CMD_GET_PACKET_STATUS, pktStatus, 4);
    _lastRSSI = -(int16_t)(pktStatus[1] / 2);
    _lastSNR = (int8_t)pktStatus[2] / 4;

    return payloadLen;
}

int16_t SX1262::getPacketRSSI() {
    return _lastRSSI;
}

int8_t SX1262::getPacketSNR() {
    return _lastSNR;
}

void SX1262::standby() {
    uint8_t mode = SX1262_STANDBY_RC;
    sendCommand(SX1262_CMD_SET_STANDBY, &mode, 1);
}

void SX1262::sleep() {
    uint8_t sleepConfig = 0x04;  // Warm start
    sendCommand(SX1262_CMD_SET_SLEEP, &sleepConfig, 1);
}

uint16_t SX1262::getDeviceErrors() {
    uint8_t errors[3];
    readCommand(SX1262_CMD_GET_DEVICE_ERRORS, errors, 3);
    return (errors[1] << 8) | errors[2];
}

uint8_t SX1262::getStatus() {
    _spi->select();
    uint8_t status = _spi->transfer(SX1262_CMD_GET_STATUS);
    _spi->transfer(0x00);  // NOP to get response
    _spi->deselect();
    return status;
}

// Private methods

void SX1262::sendCommand(uint8_t cmd) {
    _spi->waitBusy();
    _spi->select();
    _spi->transfer(cmd);
    _spi->deselect();
}

void SX1262::sendCommand(uint8_t cmd, uint8_t* data, size_t length) {
    _spi->waitBusy();
    _spi->select();
    _spi->transfer(cmd);
    for (size_t i = 0; i < length; i++) {
        _spi->transfer(data[i]);
    }
    _spi->deselect();
}

void SX1262::readCommand(uint8_t cmd, uint8_t* data, size_t length) {
    _spi->waitBusy();
    _spi->select();
    _spi->transfer(cmd);
    _spi->transfer(0x00);  // Status byte
    for (size_t i = 0; i < length; i++) {
        data[i] = _spi->transfer(0x00);
    }
    _spi->deselect();
}

void SX1262::writeRegister(uint16_t addr, uint8_t value) {
    _spi->waitBusy();
    _spi->select();
    _spi->transfer(SX1262_CMD_WRITE_REGISTER);
    _spi->transfer((addr >> 8) & 0xFF);
    _spi->transfer(addr & 0xFF);
    _spi->transfer(value);
    _spi->deselect();
}

uint8_t SX1262::readRegister(uint16_t addr) {
    _spi->waitBusy();
    _spi->select();
    _spi->transfer(SX1262_CMD_READ_REGISTER);
    _spi->transfer((addr >> 8) & 0xFF);
    _spi->transfer(addr & 0xFF);
    _spi->transfer(0x00);  // Status
    uint8_t value = _spi->transfer(0x00);
    _spi->deselect();
    return value;
}

void SX1262::writeBuffer(uint8_t offset, const uint8_t* data, size_t length) {
    _spi->waitBusy();
    _spi->select();
    _spi->transfer(SX1262_CMD_WRITE_BUFFER);
    _spi->transfer(offset);
    for (size_t i = 0; i < length; i++) {
        _spi->transfer(data[i]);
    }
    _spi->deselect();
}

void SX1262::readBuffer(uint8_t offset, uint8_t* data, size_t length) {
    _spi->waitBusy();
    _spi->select();
    _spi->transfer(SX1262_CMD_READ_BUFFER);
    _spi->transfer(offset);
    _spi->transfer(0x00);  // Status
    for (size_t i = 0; i < length; i++) {
        data[i] = _spi->transfer(0x00);
    }
    _spi->deselect();
}

void SX1262::setPacketType(uint8_t type) {
    sendCommand(SX1262_CMD_SET_PACKET_TYPE, &type, 1);
}

void SX1262::setFrequency(float freq) {
    // Convert frequency to register value
    uint32_t frf = (uint32_t)((freq * 1000000.0) / (32000000.0 / (1 << 25)));
    uint8_t data[4];
    data[0] = (frf >> 24) & 0xFF;
    data[1] = (frf >> 16) & 0xFF;
    data[2] = (frf >> 8) & 0xFF;
    data[3] = frf & 0xFF;
    sendCommand(SX1262_CMD_SET_RF_FREQUENCY, data, 4);
}

void SX1262::setModulationParams(uint8_t sf, uint8_t bw, uint8_t cr, uint8_t ldro) {
    uint8_t data[4];
    data[0] = sf;
    data[1] = bw;
    data[2] = cr;
    data[3] = ldro;
    sendCommand(SX1262_CMD_SET_MODULATION_PARAMS, data, 4);
}

void SX1262::setPacketParams(uint16_t preambleLen, bool implicitHeader, uint8_t payloadLen, bool crcOn, bool invertIQ) {
    uint8_t data[6];
    data[0] = (preambleLen >> 8) & 0xFF;
    data[1] = preambleLen & 0xFF;
    data[2] = implicitHeader ? 0x01 : 0x00;
    data[3] = payloadLen;
    data[4] = crcOn ? 0x01 : 0x00;
    data[5] = invertIQ ? 0x01 : 0x00;
    sendCommand(SX1262_CMD_SET_PACKET_PARAMS, data, 6);
}

void SX1262::setDio2AsRfSwitch(bool enable) {
    uint8_t data = enable ? 0x01 : 0x00;
    sendCommand(SX1262_CMD_SET_DIO2_AS_RF_SW_CTRL, &data, 1);
}

void SX1262::setDioIrqParams(uint16_t irqMask, uint16_t dio1Mask, uint16_t dio2Mask, uint16_t dio3Mask) {
    uint8_t data[8];
    data[0] = (irqMask >> 8) & 0xFF;
    data[1] = irqMask & 0xFF;
    data[2] = (dio1Mask >> 8) & 0xFF;
    data[3] = dio1Mask & 0xFF;
    data[4] = (dio2Mask >> 8) & 0xFF;
    data[5] = dio2Mask & 0xFF;
    data[6] = (dio3Mask >> 8) & 0xFF;
    data[7] = dio3Mask & 0xFF;
    sendCommand(SX1262_CMD_SET_DIO_IRQ_PARAMS, data, 8);
}

uint16_t SX1262::getIrqStatus() {
    uint8_t data[3];
    readCommand(SX1262_CMD_GET_IRQ_STATUS, data, 3);
    return (data[1] << 8) | data[2];
}

void SX1262::clearIrqStatus(uint16_t mask) {
    uint8_t data[2];
    data[0] = (mask >> 8) & 0xFF;
    data[1] = mask & 0xFF;
    sendCommand(SX1262_CMD_CLEAR_IRQ_STATUS, data, 2);
}

void SX1262::setBufferBaseAddress(uint8_t txBase, uint8_t rxBase) {
    uint8_t data[2];
    data[0] = txBase;
    data[1] = rxBase;
    sendCommand(SX1262_CMD_SET_BUFFER_BASE_ADDRESS, data, 2);
}

void SX1262::setRegulatorMode(uint8_t mode) {
    sendCommand(SX1262_CMD_SET_REGULATOR_MODE, &mode, 1);
}

void SX1262::calibrateImage(float freq) {
    uint8_t data[2];
    if (freq > 900) {
        data[0] = 0xE1;
        data[1] = 0xE9;
    } else if (freq > 850) {
        data[0] = 0xD7;
        data[1] = 0xDB;
    } else if (freq > 770) {
        data[0] = 0xC1;
        data[1] = 0xC5;
    } else if (freq > 460) {
        data[0] = 0x75;
        data[1] = 0x81;
    } else {
        data[0] = 0x6B;
        data[1] = 0x6F;
    }
    sendCommand(SX1262_CMD_CALIBRATE_IMAGE, data, 2);
}

uint8_t SX1262::bandwidthToReg(float bw) {
    if (bw <= 7.8) return SX1262_BW_7_8;
    if (bw <= 10.4) return SX1262_BW_10_4;
    if (bw <= 15.6) return SX1262_BW_15_6;
    if (bw <= 20.8) return SX1262_BW_20_8;
    if (bw <= 31.25) return SX1262_BW_31_25;
    if (bw <= 41.7) return SX1262_BW_41_7;
    if (bw <= 62.5) return SX1262_BW_62_5;
    if (bw <= 125) return SX1262_BW_125;
    if (bw <= 250) return SX1262_BW_250;
    return SX1262_BW_500;
}
