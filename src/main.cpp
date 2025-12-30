/**
 * LoRa Chat for M5Stack Cardputer
 * Using MCP23017 GPIO expander for SX1262 (DX-LR-30)
 *
 * Hardware connections:
 * - Cardputer Grove I2C -> MCP23017 (SDA=2, SCL=1)
 * - MCP23017 GPA0 -> DX-LR-30 NSS (Pin 9)
 * - MCP23017 GPA1 -> DX-LR-30 SCK (Pin 10)
 * - MCP23017 GPA2 -> DX-LR-30 MISO (Pin 11)
 * - MCP23017 GPA3 -> DX-LR-30 MOSI (Pin 12)
 * - MCP23017 GPA4 -> DX-LR-30 RST (Pin 17)
 * - MCP23017 GPA5 -> DX-LR-30 BUSY (Pin 21)
 */

#include <M5Cardputer.h>
#include <Preferences.h>
#include "config.h"
#include "mcp23017_spi.h"
#include "sx1262.h"
#include "chat_ui.h"

// Preferences for persistent storage
Preferences prefs;

// Packet format: [nickname_len:1][nickname:n][message:rest]
// Simple protocol without encryption for basic chat

// State machine
enum AppState {
    STATE_INIT,
    STATE_NICKNAME_SETUP,
    STATE_CHAT,
    STATE_SETTINGS
};

AppState currentState = STATE_INIT;
bool radioInitialized = false;
unsigned long lastReceiveCheck = 0;
const unsigned long RECEIVE_CHECK_INTERVAL = 50;  // ms

// Buffer for received packets
uint8_t rxBuffer[256];

// Packet builder
uint8_t txPacket[256];

/**
 * Build a chat packet
 * Format: [nickname_len][nickname][message]
 */
size_t buildPacket(const char* nickname, const char* message, uint8_t* packet) {
    size_t nickLen = strlen(nickname);
    if (nickLen > MAX_NICKNAME_LEN) nickLen = MAX_NICKNAME_LEN;

    size_t msgLen = strlen(message);
    if (msgLen > MAX_MSG_LENGTH) msgLen = MAX_MSG_LENGTH;

    packet[0] = nickLen;
    memcpy(&packet[1], nickname, nickLen);
    memcpy(&packet[1 + nickLen], message, msgLen);

    return 1 + nickLen + msgLen;
}

/**
 * Parse a received packet
 */
bool parsePacket(const uint8_t* packet, size_t length, char* nickname, char* message) {
    if (length < 2) return false;

    uint8_t nickLen = packet[0];
    if (nickLen > MAX_NICKNAME_LEN || nickLen + 1 > length) return false;

    memcpy(nickname, &packet[1], nickLen);
    nickname[nickLen] = '\0';

    size_t msgLen = length - 1 - nickLen;
    if (msgLen > MAX_MSG_LENGTH) msgLen = MAX_MSG_LENGTH;

    memcpy(message, &packet[1 + nickLen], msgLen);
    message[msgLen] = '\0';

    return true;
}

/**
 * Initialize the radio
 */
bool initRadio() {
    Serial.println("Initializing radio...");

    // Show debug on display
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setTextColor(TFT_WHITE);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setCursor(5, 10);
    M5Cardputer.Display.println("Initializing radio...");

    if (!radio.begin()) {
        Serial.println("Radio init failed!");
        M5Cardputer.Display.setTextColor(TFT_RED);
        M5Cardputer.Display.println("Radio FAILED!");
        M5Cardputer.Display.println("");
        M5Cardputer.Display.setTextColor(TFT_YELLOW);
        M5Cardputer.Display.println("Check serial 115200");
        M5Cardputer.Display.println("for details");
        delay(5000);
        return false;
    }

    // Configure LoRa parameters
    if (!radio.configure(LORA_FREQUENCY, LORA_BANDWIDTH, LORA_SPREADING, LORA_CODING_RATE)) {
        Serial.println("Radio config failed!");
        chatUI.addSystemMessage("Config FAILED!");
        return false;
    }

    // Set sync word and power
    radio.setSyncWord(LORA_SYNC_WORD);
    radio.setTxPower(LORA_TX_POWER);

    // Start receiving
    radio.startReceive();

    Serial.println("Radio ready!");
    chatUI.addSystemMessage("Radio OK!");
    chatUI.updateStatus(true, LORA_FREQUENCY);

    return true;
}

/**
 * Send a message
 */
void sendMessage(const char* message) {
    if (!radioInitialized || strlen(message) == 0) {
        return;
    }

    Serial.printf("Sending: %s\n", message);

    // Build packet
    size_t pktLen = buildPacket(chatUI.getNickname(), message, txPacket);

    // Transmit
    if (radio.transmit(txPacket, pktLen)) {
        Serial.println("TX OK");
        chatUI.addSentMessage(message);
    } else {
        Serial.println("TX FAILED");
        chatUI.addSystemMessage("TX failed!");
    }

    // Return to receive mode
    radio.startReceive();
}

/**
 * Check for received messages
 */
void checkReceive() {
    if (!radioInitialized) return;

    if (radio.available()) {
        size_t len = radio.readPacket(rxBuffer, sizeof(rxBuffer));
        if (len > 0) {
            int16_t rssi = radio.getPacketRSSI();
            int8_t snr = radio.getPacketSNR();

            Serial.printf("RX: %d bytes, RSSI=%d, SNR=%d\n", len, rssi, snr);

            // Parse packet
            char nickname[MAX_NICKNAME_LEN + 1];
            char message[MAX_MSG_LENGTH + 1];

            if (parsePacket(rxBuffer, len, nickname, message)) {
                // Don't show our own messages (in case of echo)
                if (strcmp(nickname, chatUI.getNickname()) != 0) {
                    chatUI.addReceivedMessage(nickname, message, rssi);
                }
            } else {
                Serial.println("Invalid packet format");
            }
        }

        // Continue receiving
        radio.startReceive();
    }
}

/**
 * Load settings from flash
 */
void loadSettings() {
    prefs.begin("lorachat", true);  // Read-only
    String nick = prefs.getString("nickname", "User");
    chatUI.setNickname(nick.c_str());
    prefs.end();
}

/**
 * Save settings to flash
 */
void saveSettings() {
    prefs.begin("lorachat", false);  // Read-write
    prefs.putString("nickname", chatUI.getNickname());
    prefs.end();
}

/**
 * Handle nickname setup mode
 */
void handleNicknameSetup() {
    static char newNick[MAX_NICKNAME_LEN + 1] = "";
    static int nickPos = 0;
    static bool firstDraw = true;

    if (firstDraw) {
        M5Cardputer.Display.fillScreen(COLOR_BG);
        M5Cardputer.Display.setTextColor(COLOR_MY_MSG);
        M5Cardputer.Display.setTextSize(2);
        M5Cardputer.Display.setCursor(30, 20);
        M5Cardputer.Display.println("LoRa Chat");
        M5Cardputer.Display.setTextColor(TFT_WHITE);
        M5Cardputer.Display.setTextSize(1);
        M5Cardputer.Display.setCursor(20, 60);
        M5Cardputer.Display.println("Enter your nickname:");
        M5Cardputer.Display.setCursor(20, 80);
        M5Cardputer.Display.println("(Press ENTER when done)");
        strcpy(newNick, chatUI.getNickname());
        nickPos = strlen(newNick);
        firstDraw = false;
    }

    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState keys = M5Cardputer.Keyboard.keysState();

        if (keys.enter && nickPos > 0) {
            newNick[nickPos] = '\0';
            chatUI.setNickname(newNick);
            saveSettings();
            currentState = STATE_CHAT;
            firstDraw = true;
            chatUI.redraw();
            chatUI.addSystemMessage("Welcome to LoRa Chat!");

            // Initialize radio
            radioInitialized = initRadio();
            return;
        }

        if (keys.del && nickPos > 0) {
            nickPos--;
            newNick[nickPos] = '\0';
        }

        if (keys.word.size() > 0 && nickPos < MAX_NICKNAME_LEN) {
            char c = keys.word[0];
            if (isAlphaNumeric(c) || c == '_') {
                newNick[nickPos++] = c;
                newNick[nickPos] = '\0';
            }
        }

        // Redraw nickname
        M5Cardputer.Display.fillRect(20, 100, 200, 20, COLOR_BG);
        M5Cardputer.Display.setTextColor(COLOR_OTHER_MSG);
        M5Cardputer.Display.setCursor(20, 100);
        M5Cardputer.Display.print(newNick);
        M5Cardputer.Display.print("_");
    }
}

/**
 * Handle chat mode
 */
void handleChat() {
    // Check for keyboard input
    if (chatUI.processInput()) {
        const char* msg = chatUI.getInputText();
        if (strlen(msg) > 0) {
            // Check for commands
            if (msg[0] == '/') {
                if (strncmp(msg, "/nick ", 6) == 0) {
                    // Change nickname
                    chatUI.setNickname(msg + 6);
                    saveSettings();
                    chatUI.addSystemMessage("Nickname changed");
                } else if (strcmp(msg, "/freq") == 0) {
                    // Show frequency
                    char buf[32];
                    sprintf(buf, "Freq: %.1f MHz", LORA_FREQUENCY);
                    chatUI.addSystemMessage(buf);
                } else if (strcmp(msg, "/help") == 0) {
                    chatUI.addSystemMessage("/nick <name>");
                    chatUI.addSystemMessage("/freq - show freq");
                } else {
                    chatUI.addSystemMessage("Unknown command");
                }
            } else {
                // Send message
                sendMessage(msg);
            }
            chatUI.clearInput();
        }
    }

    // Check for received messages
    if (millis() - lastReceiveCheck > RECEIVE_CHECK_INTERVAL) {
        checkReceive();
        lastReceiveCheck = millis();
    }
}

/**
 * GPIO Test Mode - helps debug wiring issues
 * Shows state of all MCP23017 pins and allows manual control
 */
void gpioTestMode() {
    auto& lcd = M5Cardputer.Display;

    Serial.println("\n=== GPIO TEST MODE ===");

    // Initialize MCP23017 only
    if (!mcpSPI.begin()) {
        lcd.fillScreen(TFT_RED);
        lcd.setTextColor(TFT_WHITE);
        lcd.setCursor(10, 60);
        lcd.println("MCP23017 FAIL!");
        while(1) delay(1000);
    }

    bool rstState = true;   // RST high (not reset)
    bool nssState = true;   // NSS high (deselected)
    bool sckState = false;  // SCK low
    bool mosiState = false; // MOSI low

    while (true) {
        lcd.fillScreen(TFT_BLACK);
        lcd.setTextSize(1);
        lcd.setTextColor(TFT_CYAN);
        lcd.setCursor(5, 5);
        lcd.println("=== GPIO TEST MODE ===");
        lcd.setTextColor(TFT_WHITE);
        lcd.setCursor(5, 20);
        lcd.printf("MCP23017 @ 0x%02X", mcpSPI.getAddress());

        // Read inputs
        bool busy = mcpSPI.isBusy();
        bool dio1 = mcpSPI.readDIO1();

        // Show OUTPUTS
        lcd.setTextColor(TFT_YELLOW);
        lcd.setCursor(5, 40);
        lcd.println("OUTPUTS (press key):");
        lcd.setTextColor(TFT_WHITE);

        lcd.setCursor(10, 55);
        lcd.printf("R: RST (PA2) = %d", rstState ? 1 : 0);
        lcd.setCursor(10, 67);
        lcd.printf("N: NSS (PA0) = %d", nssState ? 1 : 0);
        lcd.setCursor(10, 79);
        lcd.printf("S: SCK (PA3) = %d", sckState ? 1 : 0);
        lcd.setCursor(10, 91);
        lcd.printf("M: MOSI(PA6) = %d", mosiState ? 1 : 0);

        // Show INPUTS
        lcd.setTextColor(TFT_YELLOW);
        lcd.setCursor(130, 40);
        lcd.println("INPUTS:");

        lcd.setCursor(130, 55);
        lcd.setTextColor(busy ? TFT_RED : TFT_GREEN);
        lcd.printf("BUSY(PA5)=%d", busy ? 1 : 0);

        lcd.setCursor(130, 67);
        lcd.setTextColor(dio1 ? TFT_GREEN : TFT_WHITE);
        lcd.printf("DIO1(PA4)=%d", dio1 ? 1 : 0);

        // Instructions
        lcd.setTextColor(TFT_MAGENTA);
        lcd.setCursor(5, 110);
        lcd.println("T:toggle all  Q:quit");

        Serial.printf("OUT: RST=%d NSS=%d SCK=%d MOSI=%d | IN: BUSY=%d DIO1=%d\n",
                      rstState, nssState, sckState, mosiState, busy, dio1);

        // Wait for key
        delay(100);
        M5Cardputer.update();

        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            Keyboard_Class::KeysState keys = M5Cardputer.Keyboard.keysState();

            if (keys.word.size() > 0) {
                char c = keys.word[0];

                if (c == 'r' || c == 'R') {
                    rstState = !rstState;
                    mcpSPI.setReset(rstState);
                    Serial.printf("RST -> %d\n", rstState);
                }
                else if (c == 'n' || c == 'N') {
                    nssState = !nssState;
                    if (nssState) mcpSPI.deselect(); else mcpSPI.select();
                    Serial.printf("NSS -> %d\n", nssState);
                }
                else if (c == 's' || c == 'S') {
                    sckState = !sckState;
                    // Toggle SCK by doing a dummy transfer bit
                    mcpSPI.transfer(sckState ? 0xFF : 0x00);
                    Serial.printf("SCK toggled\n");
                }
                else if (c == 'm' || c == 'M') {
                    mosiState = !mosiState;
                    mcpSPI.transfer(mosiState ? 0xFF : 0x00);
                    Serial.printf("MOSI toggled\n");
                }
                else if (c == 't' || c == 'T') {
                    // Test sequence: reset pulse
                    Serial.println("=== RESET PULSE TEST ===");
                    lcd.fillScreen(TFT_BLUE);
                    lcd.setTextColor(TFT_WHITE);
                    lcd.setCursor(10, 50);
                    lcd.println("Sending RESET pulse...");
                    lcd.println("Watch BUSY pin!");

                    mcpSPI.setReset(true);  delay(100);
                    Serial.printf("Before reset: BUSY=%d\n", mcpSPI.isBusy());

                    mcpSPI.setReset(false); // RST low
                    Serial.println("RST -> LOW");
                    delay(100);

                    mcpSPI.setReset(true);  // RST high
                    Serial.println("RST -> HIGH");
                    delay(200);

                    Serial.printf("After reset: BUSY=%d\n", mcpSPI.isBusy());
                    lcd.setCursor(10, 80);
                    lcd.printf("BUSY after reset: %d", mcpSPI.isBusy());
                    delay(2000);
                }
                else if (c == 'q' || c == 'Q') {
                    return;  // Exit test mode
                }
            }
        }
    }
}

void setup() {
    // Initialize M5Cardputer
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);

    // Initialize display
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.fillScreen(COLOR_BG);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(TFT_WHITE);

    // Serial for debugging
    Serial.begin(115200);
    delay(100);

    Serial.println("\n\n=== LoRa Chat for Cardputer ===");
    Serial.println("Using MCP23017 GPIO expander");
    Serial.printf("I2C: SDA=%d, SCL=%d\n", I2C_SDA, I2C_SCL);
    Serial.printf("MCP23017 addr: 0x%02X\n", MCP23017_ADDR);

    // Check if any key is pressed for GPIO test mode
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isPressed()) {
        Serial.println("KEY PRESSED - Entering GPIO test mode!");
        gpioTestMode();
    }

    // Show splash screen
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setTextColor(COLOR_MY_MSG);
    M5Cardputer.Display.setCursor(40, 40);
    M5Cardputer.Display.println("LoRa Chat");
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(TFT_WHITE);
    M5Cardputer.Display.setCursor(30, 70);
    M5Cardputer.Display.println("MCP23017 + DX-LR-30");
    M5Cardputer.Display.setCursor(50, 90);
    M5Cardputer.Display.printf("%.1f MHz", LORA_FREQUENCY);
    M5Cardputer.Display.setCursor(20, 110);
    M5Cardputer.Display.setTextColor(TFT_YELLOW);
    M5Cardputer.Display.println("Hold key for GPIO test");

    delay(2000);

    // Debug on display
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setTextColor(TFT_WHITE);
    M5Cardputer.Display.setTextSize(1);
    int debugY = 10;

    M5Cardputer.Display.setCursor(5, debugY);
    M5Cardputer.Display.println("1. After splash...");
    debugY += 12;
    Serial.println("DEBUG: After splash delay");

    // Initialize chat UI first (creates sprite)
    M5Cardputer.Display.setCursor(5, debugY);
    M5Cardputer.Display.println("2. chatUI.begin()...");
    debugY += 12;
    Serial.println("DEBUG: Calling chatUI.begin()...");

    chatUI.begin();

    M5Cardputer.Display.setCursor(5, debugY);
    M5Cardputer.Display.println("3. chatUI OK!");
    debugY += 12;
    Serial.println("DEBUG: chatUI.begin() OK");

    // Load settings
    M5Cardputer.Display.setCursor(5, debugY);
    M5Cardputer.Display.println("4. loadSettings()...");
    debugY += 12;
    Serial.println("DEBUG: Calling loadSettings()...");

    loadSettings();

    M5Cardputer.Display.setCursor(5, debugY);
    M5Cardputer.Display.println("5. Settings OK!");
    debugY += 12;
    Serial.println("DEBUG: loadSettings() OK");

    M5Cardputer.Display.setCursor(5, debugY);
    M5Cardputer.Display.println("6. Starting loop...");
    Serial.println("DEBUG: Entering STATE_NICKNAME_SETUP");

    delay(1000);  // Show debug for 1 second

    // Go to nickname setup
    currentState = STATE_NICKNAME_SETUP;
}

void loop() {
    static bool firstLoop = true;
    if (firstLoop) {
        Serial.println("DEBUG: First loop() iteration");
        firstLoop = false;
    }

    switch (currentState) {
        case STATE_INIT:
            // Should not reach here
            currentState = STATE_NICKNAME_SETUP;
            break;

        case STATE_NICKNAME_SETUP:
            handleNicknameSetup();
            break;

        case STATE_CHAT:
            handleChat();
            break;

        case STATE_SETTINGS:
            // TODO: Settings menu
            currentState = STATE_CHAT;
            break;
    }

    delay(10);  // Small delay to prevent watchdog issues
}
