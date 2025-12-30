/**
 * MeshtasticDIY - LoRa Chat for ESP32-S3 + DX-LR-30 (SX1262)
 * With 2" SPI Display (ST7789)
 *
 * Hardware: ESP32-S3 WROOM + DX-LR-30 + Display SPI 2"
 * Uses hardware SPI for fast communication
 * Meshtastic-compatible radio settings (LONG_FAST preset)
 *
 * Wiring LoRa (SPI HSPI):
 *   ESP32-S3    DX-LR-30
 *   GPIO10  ->  Pin 9  (NSS)
 *   GPIO12  ->  Pin 10 (SCK)
 *   GPIO13  ->  Pin 11 (MISO)
 *   GPIO11  ->  Pin 12 (MOSI)
 *   GPIO2   ->  Pin 13 (DIO1)
 *   GPIO3   ->  Pin 17 (RST)
 *   GPIO4   ->  Pin 21 (BUSY)
 *   3.3V    ->  Pin 1  (VCC)
 *   GND     ->  Pin 2  (GND)
 *
 * Wiring Display SPI 2" (SPI VSPI):
 *   ESP32-S3    Display
 *   GPIO35  ->  SCK (CLK)
 *   GPIO37  ->  MOSI (SDA/DIN)
 *   GPIO36  ->  CS
 *   GPIO38  ->  DC (RS)
 *   GPIO39  ->  RST
 *   GPIO40  ->  BL (Backlight, optional)
 *   3.3V    ->  VCC
 *   GND     ->  GND
 */

#include <SPI.h>
#include <RadioLib.h>
#include <TFT_eSPI.h>  // Install via Library Manager

// ==================== LoRa SPI Pins (HSPI) ====================
#define LORA_SCK    12
#define LORA_MOSI   11
#define LORA_MISO   13
#define LORA_NSS    10

// SX1262 Control Pins
#define LORA_RST    3
#define LORA_BUSY   4
#define LORA_DIO1   2

// ==================== Display SPI Pins (VSPI) ====================
// Configure these in User_Setup.h of TFT_eSPI library!
// Or use these defines (need library modification)
#define TFT_SCK     35
#define TFT_MOSI    37
#define TFT_CS      36
#define TFT_DC      38
#define TFT_RST     39
#define TFT_BL      40

// Display dimensions (2" typically 240x320 or 320x240)
#define TFT_WIDTH   240
#define TFT_HEIGHT  320

// ==================== LoRa Configuration ====================
// Meshtastic LONG_FAST preset
#define LORA_FREQUENCY      868.0   // MHz (EU: 868, US: 915)
#define LORA_BANDWIDTH      250.0   // kHz
#define LORA_SPREADING      11      // SF11
#define LORA_CODING_RATE    5       // 4/5
#define LORA_SYNC_WORD      0x2B    // Meshtastic public
#define LORA_TX_POWER       14      // dBm
#define LORA_PREAMBLE       16      // symbols

// ==================== Chat Configuration ====================
#define MAX_MSG_LEN         200
#define MAX_NICK_LEN        16
#define MAX_MESSAGES        10
#define FONT_HEIGHT         16

String myNickname = "User";

// Message history
struct ChatMessage {
    String sender;
    String text;
    int16_t rssi;
    bool isMine;
};
ChatMessage messages[MAX_MESSAGES];
int messageCount = 0;
String inputBuffer = "";

// ==================== Hardware Instances ====================
SPIClass radioSPI(HSPI);
SX1262 radio = new Module(LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY, radioSPI);
TFT_eSPI tft = TFT_eSPI();

// ==================== State ====================
volatile bool receivedFlag = false;
volatile bool transmittedFlag = false;
bool transmitMode = false;
bool displayEnabled = true;

// ==================== Colors ====================
#define COLOR_BG        TFT_BLACK
#define COLOR_TEXT      TFT_WHITE
#define COLOR_MY_MSG    TFT_CYAN
#define COLOR_OTHER_MSG TFT_GREEN
#define COLOR_SYSTEM    TFT_YELLOW
#define COLOR_ERROR     TFT_RED
#define COLOR_INPUT_BG  0x1082

// ISR for DIO1
void IRAM_ATTR onDio1Action() {
    if (transmitMode) {
        transmittedFlag = true;
    } else {
        receivedFlag = true;
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n========================================");
    Serial.println("   MeshtasticDIY - ESP32-S3 + DX-LR-30");
    Serial.println("========================================\n");

    // Initialize Display
    initDisplay();

    // Initialize LoRa SPI
    radioSPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);

    // Initialize Radio
    showStatus("Init LoRa...", COLOR_SYSTEM);
    Serial.print("[SX1262] Initializing... ");

    int state = radio.begin(
        LORA_FREQUENCY,
        LORA_BANDWIDTH,
        LORA_SPREADING,
        LORA_CODING_RATE,
        LORA_SYNC_WORD,
        LORA_TX_POWER,
        LORA_PREAMBLE
    );

    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("OK!");
        showStatus("LoRa OK!", COLOR_MY_MSG);
    } else {
        Serial.printf("FAILED! Error: %d\n", state);
        showStatus("LoRa FAILED!", COLOR_ERROR);
        showError(state);
        while (true) delay(1000);
    }

    // Configure radio
    radio.setDio1Action(onDio1Action);
    radio.setCurrentLimit(140.0);
    radio.setDio2AsRfSwitch(true);

    // Show config
    Serial.println("\nRadio Configuration:");
    Serial.printf("  Frequency: %.1f MHz\n", LORA_FREQUENCY);
    Serial.printf("  Bandwidth: %.0f kHz\n", LORA_BANDWIDTH);
    Serial.printf("  Spreading: SF%d\n", LORA_SPREADING);
    Serial.printf("  TX Power:  %d dBm\n", LORA_TX_POWER);

    // Start receiving
    radio.startReceive();
    transmitMode = false;

    // Draw chat UI
    delay(1000);
    drawChatUI();
    addSystemMessage("Radio OK - " + String(LORA_FREQUENCY, 1) + " MHz");

    Serial.println("\nReady! Type message and press Enter.");
    Serial.print("> ");
}

void loop() {
    // Check for received packets
    if (receivedFlag) {
        receivedFlag = false;
        handleReceive();
    }

    // Check for transmitted packets
    if (transmittedFlag) {
        transmittedFlag = false;
        handleTransmitDone();
    }

    // Check for Serial input
    if (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (inputBuffer.length() > 0) {
                handleInput(inputBuffer);
                inputBuffer = "";
            }
            Serial.print("> ");
        } else if (c == 8 || c == 127) {  // Backspace
            if (inputBuffer.length() > 0) {
                inputBuffer.remove(inputBuffer.length() - 1);
            }
        } else if (c >= 32 && c < 127) {
            inputBuffer += c;
        }
        updateInputDisplay();
    }
}

void initDisplay() {
    // Backlight
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    tft.init();
    tft.setRotation(1);  // Landscape
    tft.fillScreen(COLOR_BG);
    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(1);

    // Splash screen
    tft.setTextColor(COLOR_MY_MSG);
    tft.setTextSize(2);
    tft.setCursor(60, 100);
    tft.println("MeshtasticDIY");
    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(1);
    tft.setCursor(80, 130);
    tft.println("ESP32-S3 + DX-LR-30");
    tft.setCursor(95, 150);
    tft.printf("%.1f MHz", LORA_FREQUENCY);

    displayEnabled = true;
}

void showStatus(String msg, uint16_t color) {
    if (!displayEnabled) return;
    tft.fillRect(0, 200, 320, 20, COLOR_BG);
    tft.setTextColor(color);
    tft.setCursor(10, 205);
    tft.print(msg);
}

void showError(int code) {
    tft.fillScreen(COLOR_ERROR);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(50, 80);
    tft.println("LoRa ERROR");
    tft.setTextSize(1);
    tft.setCursor(50, 120);
    tft.printf("Code: %d", code);
    tft.setCursor(20, 150);
    tft.println("Check wiring:");
    tft.setCursor(20, 170);
    tft.println("NSS=10 SCK=12 MISO=13 MOSI=11");
    tft.setCursor(20, 185);
    tft.println("RST=3 BUSY=4 DIO1=2");
}

void drawChatUI() {
    tft.fillScreen(COLOR_BG);

    // Header
    tft.fillRect(0, 0, 320, 20, COLOR_INPUT_BG);
    tft.setTextColor(COLOR_MY_MSG);
    tft.setCursor(5, 5);
    tft.print("MeshtasticDIY");
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(200, 5);
    tft.printf("%.1f MHz SF%d", LORA_FREQUENCY, LORA_SPREADING);

    // Input area
    tft.fillRect(0, 220, 320, 20, COLOR_INPUT_BG);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(5, 225);
    tft.print("> ");
}

void updateInputDisplay() {
    if (!displayEnabled) return;
    tft.fillRect(20, 220, 300, 20, COLOR_INPUT_BG);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(20, 225);
    String display = inputBuffer;
    if (display.length() > 35) {
        display = display.substring(display.length() - 35);
    }
    tft.print(display);
    tft.print("_");
}

void addMessage(String sender, String text, int16_t rssi, bool isMine) {
    // Shift messages up
    if (messageCount >= MAX_MESSAGES) {
        for (int i = 0; i < MAX_MESSAGES - 1; i++) {
            messages[i] = messages[i + 1];
        }
        messageCount = MAX_MESSAGES - 1;
    }

    messages[messageCount].sender = sender;
    messages[messageCount].text = text;
    messages[messageCount].rssi = rssi;
    messages[messageCount].isMine = isMine;
    messageCount++;

    redrawMessages();
}

void addSystemMessage(String text) {
    addMessage("*", text, 0, false);
}

void redrawMessages() {
    if (!displayEnabled) return;

    // Clear message area
    tft.fillRect(0, 22, 320, 195, COLOR_BG);

    int y = 25;
    int maxVisible = 12;
    int start = messageCount > maxVisible ? messageCount - maxVisible : 0;

    for (int i = start; i < messageCount; i++) {
        ChatMessage& m = messages[i];

        if (m.sender == "*") {
            // System message
            tft.setTextColor(COLOR_SYSTEM);
            tft.setCursor(5, y);
            tft.print("* ");
            tft.print(m.text);
        } else {
            // Chat message
            tft.setTextColor(m.isMine ? COLOR_MY_MSG : COLOR_OTHER_MSG);
            tft.setCursor(5, y);
            tft.print("<");
            tft.print(m.sender);
            tft.print("> ");
            tft.setTextColor(COLOR_TEXT);
            tft.print(m.text);

            if (!m.isMine && m.rssi != 0) {
                tft.setTextColor(COLOR_SYSTEM);
                tft.printf(" [%d]", m.rssi);
            }
        }

        y += FONT_HEIGHT;
        if (y > 210) break;
    }
}

void handleReceive() {
    String msg;
    int state = radio.readData(msg);

    if (state == RADIOLIB_ERR_NONE) {
        float rssi = radio.getRSSI();
        float snr = radio.getSNR();

        Serial.printf("\n[RX] RSSI: %.1f dBm, SNR: %.1f dB\n", rssi, snr);

        // Parse chat message [nick:message]
        int colonPos = msg.indexOf(':');
        if (colonPos > 0 && colonPos < MAX_NICK_LEN) {
            String nick = msg.substring(0, colonPos);
            String text = msg.substring(colonPos + 1);
            Serial.printf("<%s> %s\n", nick.c_str(), text.c_str());

            if (nick != myNickname) {
                addMessage(nick, text, (int16_t)rssi, false);
            }
        } else {
            // Raw packet
            Serial.print("[RAW] ");
            String hexStr = "";
            for (int i = 0; i < min((int)msg.length(), 20); i++) {
                char buf[4];
                sprintf(buf, "%02X ", (uint8_t)msg[i]);
                hexStr += buf;
            }
            Serial.println(hexStr);
            addMessage("RAW", hexStr, (int16_t)rssi, false);
        }
    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
        Serial.println("[RX] CRC Error");
    }

    radio.startReceive();
    transmitMode = false;
    Serial.print("> ");
}

void handleTransmitDone() {
    Serial.println("[TX] Done!");
    radio.startReceive();
    transmitMode = false;
}

void handleInput(String input) {
    if (input.startsWith("/nick ")) {
        myNickname = input.substring(6);
        myNickname.trim();
        if (myNickname.length() > MAX_NICK_LEN) {
            myNickname = myNickname.substring(0, MAX_NICK_LEN);
        }
        Serial.printf("Nickname: %s\n", myNickname.c_str());
        addSystemMessage("Nick: " + myNickname);
    }
    else if (input == "/ping") {
        sendMessage("PING");
    }
    else if (input == "/info") {
        addSystemMessage("Freq: " + String(LORA_FREQUENCY, 1) + " MHz");
        addSystemMessage("Preset: LONG_FAST");
        addSystemMessage("Nick: " + myNickname);
    }
    else if (input == "/clear") {
        messageCount = 0;
        drawChatUI();
    }
    else if (input.startsWith("/")) {
        addSystemMessage("? /nick /ping /info /clear");
    }
    else {
        sendMessage(input);
    }
}

void sendMessage(String text) {
    String packet = myNickname + ":" + text;

    if (packet.length() > MAX_MSG_LEN) {
        packet = packet.substring(0, MAX_MSG_LEN);
    }

    Serial.printf("[TX] <%s> %s\n", myNickname.c_str(), text.c_str());
    addMessage(myNickname, text, 0, true);

    transmitMode = true;
    int state = radio.startTransmit(packet);

    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[TX] Failed! Error: %d\n", state);
        addSystemMessage("TX Failed!");
        transmitMode = false;
        radio.startReceive();
    }
}
