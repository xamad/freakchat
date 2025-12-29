/**
 * Chat UI for M5Stack Cardputer
 * Optimized for 240x135 display and built-in keyboard
 */

#ifndef CHAT_UI_H
#define CHAT_UI_H

#include <Arduino.h>
#include <M5Cardputer.h>
#include "config.h"

// Color scheme (RGB565)
#define COLOR_BG            TFT_BLACK
#define COLOR_TEXT          TFT_WHITE
#define COLOR_MY_MSG        TFT_GREEN
#define COLOR_OTHER_MSG     TFT_CYAN
#define COLOR_SYSTEM_MSG    TFT_YELLOW
#define COLOR_INPUT_BG      0x2104      // Dark gray
#define COLOR_INPUT_TEXT    TFT_WHITE
#define COLOR_HEADER_BG     0x000F      // Dark blue
#define COLOR_HEADER_TEXT   TFT_WHITE
#define COLOR_STATUS_OK     TFT_GREEN
#define COLOR_STATUS_ERR    TFT_RED
#define COLOR_RSSI_GOOD     TFT_GREEN
#define COLOR_RSSI_MED      TFT_YELLOW
#define COLOR_RSSI_BAD      TFT_RED

// UI Layout (240x135 display)
#define HEADER_HEIGHT       14
#define INPUT_HEIGHT        16
#define MSG_AREA_Y          HEADER_HEIGHT
#define MSG_AREA_HEIGHT     (DISPLAY_HEIGHT - HEADER_HEIGHT - INPUT_HEIGHT)
#define MSG_FONT_SIZE       1
#define INPUT_Y             (DISPLAY_HEIGHT - INPUT_HEIGHT)

// Maximum messages in history
#define MAX_MESSAGES        MSG_HISTORY_SIZE

// Message structure
struct ChatMessage {
    char nickname[MAX_NICKNAME_LEN + 1];
    char text[MAX_MSG_LENGTH + 1];
    int16_t rssi;
    bool isMine;
    bool isSystem;
};

class ChatUI {
public:
    ChatUI();

    /**
     * Initialize the UI
     */
    void begin();

    /**
     * Set the local nickname
     * @param nickname User's nickname
     */
    void setNickname(const char* nickname);

    /**
     * Update status indicators
     * @param connected Radio connected
     * @param frequency Current frequency in MHz
     */
    void updateStatus(bool connected, float frequency);

    /**
     * Add a received message
     * @param nickname Sender's nickname
     * @param text Message text
     * @param rssi RSSI value
     */
    void addReceivedMessage(const char* nickname, const char* text, int16_t rssi);

    /**
     * Add a sent message (own message)
     * @param text Message text
     */
    void addSentMessage(const char* text);

    /**
     * Add a system message
     * @param text System message
     */
    void addSystemMessage(const char* text);

    /**
     * Process keyboard input
     * @return true if a message is ready to send
     */
    bool processInput();

    /**
     * Get the current input text
     * @return Input buffer
     */
    const char* getInputText() const { return _inputBuffer; }

    /**
     * Clear the input buffer
     */
    void clearInput();

    /**
     * Get the current nickname
     * @return Nickname
     */
    const char* getNickname() const { return _nickname; }

    /**
     * Check if in settings mode
     * @return true if in settings
     */
    bool isInSettings() const { return _inSettings; }

    /**
     * Redraw the entire screen
     */
    void redraw();

    /**
     * Show RSSI indicator for last received message
     * @param rssi RSSI value
     */
    void showRSSI(int16_t rssi);

private:
    M5Canvas _canvas;
    char _nickname[MAX_NICKNAME_LEN + 1];
    char _inputBuffer[MAX_MSG_LENGTH + 1];
    size_t _inputPos;
    ChatMessage _messages[MAX_MESSAGES];
    size_t _messageCount;
    size_t _scrollPos;
    bool _connected;
    float _frequency;
    bool _inSettings;
    bool _needsRedraw;
    unsigned long _lastCursorBlink;
    bool _cursorVisible;

    /**
     * Draw the header bar
     */
    void drawHeader();

    /**
     * Draw the message area
     */
    void drawMessages();

    /**
     * Draw the input area
     */
    void drawInput();

    /**
     * Add a message to history
     * @param msg Message to add
     */
    void addMessage(const ChatMessage& msg);

    /**
     * Scroll messages if needed
     */
    void scrollToBottom();

    /**
     * Draw RSSI bar
     * @param x X position
     * @param y Y position
     * @param rssi RSSI value
     */
    void drawRSSIBar(int x, int y, int16_t rssi);

    /**
     * Handle special keys (backspace, enter, etc.)
     * @param key Key code
     * @return true if key was handled
     */
    bool handleSpecialKey(char key);

    /**
     * Word wrap a string for display
     * @param text Input text
     * @param maxWidth Maximum width in pixels
     * @param lines Output array for wrapped lines
     * @param maxLines Maximum number of lines
     * @return Number of lines
     */
    int wrapText(const char* text, int maxWidth, String* lines, int maxLines);
};

// Global UI instance
extern ChatUI chatUI;

#endif // CHAT_UI_H
