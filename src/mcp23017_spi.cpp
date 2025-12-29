/**
 * Bit-banged SPI implementation via MCP23017 GPIO expander
 * Direct I2C register access (no external library needed)
 */

#include "mcp23017_spi.h"
#include <M5Cardputer.h>

// Global instance
MCP23017_SPI mcpSPI;

MCP23017_SPI::MCP23017_SPI()
    : _initialized(false)
    , _i2cAddr(0)
    , _portA_output(0)
{
}

bool MCP23017_SPI::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(_i2cAddr);
    Wire.write(reg);
    Wire.write(value);
    uint8_t error = Wire.endTransmission();
    if (error != 0) {
        Serial.printf("I2C write error: %d (reg=0x%02X, val=0x%02X)\n", error, reg, value);
        return false;
    }
    return true;
}

uint8_t MCP23017_SPI::readRegister(uint8_t reg) {
    Wire.beginTransmission(_i2cAddr);
    Wire.write(reg);
    uint8_t error = Wire.endTransmission(false);  // Repeated start
    if (error != 0) {
        Serial.printf("I2C read error (TX): %d (reg=0x%02X)\n", error, reg);
        return 0xFF;
    }

    uint8_t count = Wire.requestFrom(_i2cAddr, (uint8_t)1);
    if (count != 1) {
        Serial.printf("I2C read error (RX): got %d bytes\n", count);
        return 0xFF;
    }

    return Wire.read();
}

bool MCP23017_SPI::begin() {
    auto& lcd = M5Cardputer.Display;

    Serial.println("\n=== MCP23017_SPI::begin() ===");
    Serial.printf("I2C pins: SDA=%d, SCL=%d\n", I2C_SDA, I2C_SCL);

    lcd.fillScreen(TFT_BLACK);
    lcd.setTextColor(TFT_WHITE);
    lcd.setTextSize(1);
    lcd.setCursor(5, 5);
    lcd.println("MCP23017 Init");
    lcd.println();

    // Initialize I2C with explicit pins
    // End any existing Wire connection first
    Wire.end();
    delay(10);

    // Start I2C on Grove port
    bool wireOK = Wire.begin(I2C_SDA, I2C_SCL);
    Serial.printf("Wire.begin() returned: %d\n", wireOK);
    lcd.printf("Wire.begin: %s\n", wireOK ? "OK" : "FAIL");

    if (!wireOK) {
        lcd.setTextColor(TFT_RED);
        lcd.println("I2C init failed!");
        delay(3000);
        return false;
    }

    Wire.setClock(100000);  // 100kHz for reliability
    delay(50);  // Let I2C settle

    // Scan I2C bus
    Serial.println("Scanning I2C bus...");
    lcd.print("I2C scan: ");

    _i2cAddr = 0;
    for (uint8_t addr = 0x20; addr <= 0x27; addr++) {
        Wire.beginTransmission(addr);
        uint8_t error = Wire.endTransmission();
        Serial.printf("  0x%02X: %s\n", addr, (error == 0) ? "FOUND" : "---");
        if (error == 0) {
            _i2cAddr = addr;
            Serial.printf("  -> Using 0x%02X\n", _i2cAddr);
            break;
        }
    }

    if (_i2cAddr == 0) {
        Serial.println("MCP23017 NOT FOUND!");
        lcd.setTextColor(TFT_RED);
        lcd.println("NONE!");
        lcd.println();
        lcd.setTextColor(TFT_YELLOW);
        lcd.println("Check wiring:");
        lcd.println(" SDA -> G2");
        lcd.println(" SCL -> G1");
        lcd.println(" VCC -> 3.3V");
        lcd.println(" GND -> GND");
        delay(5000);
        return false;
    }

    lcd.setTextColor(TFT_GREEN);
    lcd.printf("0x%02X\n", _i2cAddr);
    Serial.printf("MCP23017 found at 0x%02X\n", _i2cAddr);

    // Now configure MCP23017 registers directly
    lcd.setTextColor(TFT_WHITE);
    lcd.print("Config regs...");

    // Read IOCON to verify communication
    uint8_t iocon = readRegister(MCP_IOCON);
    Serial.printf("IOCON read: 0x%02X\n", iocon);

    // Configure IOCON: disable sequential operation, active-low interrupt
    if (!writeRegister(MCP_IOCON, 0x20)) {
        lcd.setTextColor(TFT_RED);
        lcd.println("FAIL");
        Serial.println("Failed to write IOCON");
        delay(3000);
        return false;
    }

    // Verify IOCON was written
    iocon = readRegister(MCP_IOCON);
    Serial.printf("IOCON verify: 0x%02X\n", iocon);

    // Configure Port A direction
    // Outputs: NSS(0), SCK(3), RST(2), MOSI(6)
    // Inputs:  MISO(1), DIO1(4), BUSY(5)
    uint8_t dirA = 0xFF;  // Start with all inputs
    dirA &= ~(1 << MCP_LORA_NSS);   // Output
    dirA &= ~(1 << MCP_LORA_SCK);   // Output
    dirA &= ~(1 << MCP_LORA_RST);   // Output
    dirA &= ~(1 << MCP_LORA_MOSI);  // Output

    Serial.printf("IODIRA setting: 0x%02X\n", dirA);
    Serial.printf("  Outputs: NSS=%d, SCK=%d, RST=%d, MOSI=%d\n",
                  MCP_LORA_NSS, MCP_LORA_SCK, MCP_LORA_RST, MCP_LORA_MOSI);
    Serial.printf("  Inputs:  MISO=%d, DIO1=%d, BUSY=%d\n",
                  MCP_LORA_MISO, MCP_LORA_DIO1, MCP_LORA_BUSY);

    if (!writeRegister(MCP_IODIRA, dirA)) {
        lcd.setTextColor(TFT_RED);
        lcd.println("DIR FAIL");
        Serial.println("Failed to write IODIRA");
        delay(3000);
        return false;
    }

    // Verify direction register
    uint8_t dirCheck = readRegister(MCP_IODIRA);
    Serial.printf("IODIRA verify: 0x%02X (expected 0x%02X)\n", dirCheck, dirA);

    if (dirCheck != dirA) {
        lcd.setTextColor(TFT_RED);
        lcd.println("DIR MISMATCH");
        Serial.println("IODIRA mismatch!");
        delay(3000);
        return false;
    }

    // Enable pull-ups on input pins
    uint8_t pullupA = (1 << MCP_LORA_MISO) | (1 << MCP_LORA_DIO1) | (1 << MCP_LORA_BUSY);
    Serial.printf("GPPUA setting: 0x%02X\n", pullupA);
    writeRegister(MCP_GPPUA, pullupA);

    lcd.setTextColor(TFT_GREEN);
    lcd.println("OK");

    // Set initial output states
    lcd.setTextColor(TFT_WHITE);
    lcd.print("Init outputs...");

    _portA_output = 0;
    _portA_output |= (1 << MCP_LORA_NSS);   // CS high (deselected)
    _portA_output |= (1 << MCP_LORA_RST);   // RST high (not reset)
    // SCK low, MOSI low

    Serial.printf("Initial OLATA: 0x%02X\n", _portA_output);

    if (!writeRegister(MCP_OLATA, _portA_output)) {
        lcd.setTextColor(TFT_RED);
        lcd.println("FAIL");
        Serial.println("Failed to write OLATA");
        delay(3000);
        return false;
    }

    // Verify outputs
    uint8_t olatCheck = readRegister(MCP_OLATA);
    Serial.printf("OLATA verify: 0x%02X\n", olatCheck);

    lcd.setTextColor(TFT_GREEN);
    lcd.println("OK");

    // Read inputs to verify
    lcd.setTextColor(TFT_WHITE);
    lcd.print("Read GPIO...");

    uint8_t gpioA = readRegister(MCP_GPIOA);
    Serial.printf("GPIOA read: 0x%02X\n", gpioA);
    Serial.printf("  BUSY=%d, DIO1=%d, MISO=%d\n",
                  (gpioA >> MCP_LORA_BUSY) & 1,
                  (gpioA >> MCP_LORA_DIO1) & 1,
                  (gpioA >> MCP_LORA_MISO) & 1);

    lcd.setTextColor(TFT_GREEN);
    lcd.printf("0x%02X\n", gpioA);

    lcd.setTextColor(TFT_CYAN);
    lcd.printf("BUSY=%d\n", (gpioA >> MCP_LORA_BUSY) & 1);

    _initialized = true;
    Serial.println("MCP23017 initialization complete!");

    delay(1000);
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
    return (readInputs() & (1 << MCP_LORA_BUSY)) != 0;
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
    return (readInputs() & (1 << MCP_LORA_DIO1)) != 0;
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
    writeRegister(MCP_OLATA, _portA_output);
}

uint8_t MCP23017_SPI::readInputs() {
    return readRegister(MCP_GPIOA);
}
