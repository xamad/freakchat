/**
 * Bit-banged SPI implementation via MCP23017 GPIO expander
 */

#include "mcp23017_spi.h"
#include <M5Cardputer.h>

// Global instance
MCP23017_SPI mcpSPI;

MCP23017_SPI::MCP23017_SPI()
    : _initialized(false)
    , _portA_output(0)
{
}

bool MCP23017_SPI::begin() {
    Serial.println("MCP23017_SPI::begin() starting...");
    Serial.printf("I2C pins: SDA=%d, SCL=%d\n", I2C_SDA, I2C_SCL);

    auto& lcd = M5Cardputer.Display;

    // Initialize I2C
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(100000);
    Serial.println("I2C initialized");

    // Scan I2C bus
    Serial.println("Scanning I2C bus...");
    lcd.println("I2C Scan:");
    int found = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("  Found: 0x%02X\n", addr);
            lcd.printf(" 0x%02X", addr);
            found++;
        }
    }
    if (found == 0) {
        Serial.println("  No I2C devices found!");
        lcd.setTextColor(TFT_RED);
        lcd.println(" NONE!");
        lcd.println("Check I2C wiring!");
        delay(3000);
        return false;
    }
    lcd.println("");

    // Initialize MCP23017
    Serial.printf("Looking for MCP23017 at 0x%02X...\n", MCP23017_ADDR);
    lcd.printf("MCP@0x%02X...", MCP23017_ADDR);
    if (!_mcp.begin_I2C(MCP23017_ADDR, &Wire)) {
        Serial.println("MCP23017 NOT FOUND!");
        lcd.setTextColor(TFT_RED);
        lcd.println("FAIL");
        delay(3000);
        return false;
    }

    lcd.setTextColor(TFT_GREEN);
    lcd.println("OK");
    Serial.println("MCP23017 FOUND!");

    // Configure pins
    Serial.println("Configuring GPIO pins...");
    Serial.printf("NSS=%d, MISO=%d, SCK=%d, RST=%d, DIO1=%d, BUSY=%d, MOSI=%d\n",
                  MCP_LORA_NSS, MCP_LORA_MISO, MCP_LORA_SCK, MCP_LORA_RST,
                  MCP_LORA_DIO1, MCP_LORA_BUSY, MCP_LORA_MOSI);

    // Output pins: NSS, SCK, MOSI, RST
    _mcp.pinMode(MCP_LORA_NSS, OUTPUT);
    _mcp.pinMode(MCP_LORA_SCK, OUTPUT);
    _mcp.pinMode(MCP_LORA_MOSI, OUTPUT);
    _mcp.pinMode(MCP_LORA_RST, OUTPUT);

    // Input pins: MISO, BUSY, DIO1
    _mcp.pinMode(MCP_LORA_MISO, INPUT);
    _mcp.pinMode(MCP_LORA_BUSY, INPUT);
    _mcp.pinMode(MCP_LORA_DIO1, INPUT);

    Serial.println("GPIO configured");

    // Set initial states
    _portA_output = 0;
    _portA_output |= (1 << MCP_LORA_NSS);   // CS high (deselected)
    _portA_output |= (1 << MCP_LORA_RST);   // RST high (not reset)
    updateOutputs();

    Serial.println("Initial states set");

    // Read back to verify
    uint8_t inputs = _mcp.readGPIOA();
    Serial.printf("GPIOA read: 0x%02X (BUSY=%d)\n", inputs, (inputs >> MCP_LORA_BUSY) & 1);

    _initialized = true;
    Serial.println("MCP23017 SPI ready!");
    return true;
}

void MCP23017_SPI::select() {
    _portA_output &= ~(1 << MCP_LORA_NSS);  // CS low
    updateOutputs();
}

void MCP23017_SPI::deselect() {
    _portA_output |= (1 << MCP_LORA_NSS);   // CS high
    updateOutputs();
}

void MCP23017_SPI::setReset(bool state) {
    if (state) {
        _portA_output |= (1 << MCP_LORA_RST);
    } else {
        _portA_output &= ~(1 << MCP_LORA_RST);
    }
    updateOutputs();
}

bool MCP23017_SPI::isBusy() {
    return (_mcp.readGPIOA() & (1 << MCP_LORA_BUSY)) != 0;
}

bool MCP23017_SPI::waitBusy(uint32_t timeout_ms) {
    uint32_t start = millis();
    while (isBusy()) {
        if (millis() - start > timeout_ms) {
            Serial.println("BUSY timeout!");
            return false;
        }
        delayMicroseconds(100);
    }
    return true;
}

bool MCP23017_SPI::readDIO1() {
    return (_mcp.readGPIOA() & (1 << MCP_LORA_DIO1)) != 0;
}

uint8_t MCP23017_SPI::transferBit(uint8_t bit) {
    // Set MOSI
    if (bit) {
        _portA_output |= (1 << MCP_LORA_MOSI);
    } else {
        _portA_output &= ~(1 << MCP_LORA_MOSI);
    }

    // Clock low, set data
    _portA_output &= ~(1 << MCP_LORA_SCK);
    updateOutputs();

    // Small delay
    delayMicroseconds(SPI_DELAY_US);

    // Clock high, sample data
    _portA_output |= (1 << MCP_LORA_SCK);
    updateOutputs();

    // Read MISO
    uint8_t inputs = readInputs();

    delayMicroseconds(SPI_DELAY_US);

    // Return the MISO bit
    return (inputs & (1 << MCP_LORA_MISO)) ? 1 : 0;
}

uint8_t MCP23017_SPI::transfer(uint8_t data) {
    uint8_t received = 0;

    // SPI Mode 0: CPOL=0, CPHA=0
    // Data sampled on rising edge, shifted out on falling edge
    for (int8_t i = 7; i >= 0; i--) {
        uint8_t bit = (data >> i) & 0x01;
        uint8_t rxBit = transferBit(bit);
        received |= (rxBit << i);
    }

    // Clock low at end
    _portA_output &= ~(1 << MCP_LORA_SCK);
    updateOutputs();

    return received;
}

void MCP23017_SPI::transfer(uint8_t* buffer, size_t length) {
    for (size_t i = 0; i < length; i++) {
        buffer[i] = transfer(buffer[i]);
    }
}

void MCP23017_SPI::write(const uint8_t* buffer, size_t length) {
    for (size_t i = 0; i < length; i++) {
        transfer(buffer[i]);
    }
}

void MCP23017_SPI::read(uint8_t* buffer, size_t length) {
    for (size_t i = 0; i < length; i++) {
        buffer[i] = transfer(0x00);
    }
}

void MCP23017_SPI::updateOutputs() {
    _mcp.writeGPIOA(_portA_output);
}

uint8_t MCP23017_SPI::readInputs() {
    return _mcp.readGPIOA();
}
