/**
 * Chat UI implementation for M5Stack Cardputer
 * Simplified version using direct display access
 */

#include "chat_ui.h"

// Global instance
ChatUI chatUI;

// Reference to display for convenience
#define LCD M5Cardputer.Display

ChatUI::ChatUI()
    : _initialized(false)
    , _inputPos(0)
    , _messageCount(0)
    , _scrollPos(0)
    , _connected(false)
    , _frequency(LORA_FREQUENCY)
    , _inSettings(false)
    , _needsRedraw(true)
    , _lastCursorBlink(0)
    , _cursorVisible(true)
{
    memset(_nickname, 0, sizeof(_nickname));
    memset(_inputBuffer, 0, sizeof(_inputBuffer));
    memset(_messages, 0, sizeof(_messages));
    strcpy(_nickname, "User");
}

void ChatUI::begin() {
    Serial.println("DEBUG: ChatUI::begin()");
    _initialized = true;
    // Don't redraw here - let the caller do it
}

void ChatUI::setNickname(const char* nickname) {
    strncpy(_nickname, nickname, MAX_NICKNAME_LEN);
    _nickname[MAX_NICKNAME_LEN] = '\0';
    if (_initialized) {
        drawHeader();
    }
}

void ChatUI::updateStatus(bool connected, float frequency) {
    if (_connected != connected || _frequency != frequency) {
        _connected = connected;
        _frequency = frequency;
        if (_initialized) {
            drawHeader();
        }
    }
}

void ChatUI::addReceivedMessage(const char* nickname, const char* text, int16_t rssi) {
    ChatMessage msg;
    strncpy(msg.nickname, nickname, MAX_NICKNAME_LEN);
    msg.nickname[MAX_NICKNAME_LEN] = '\0';
    strncpy(msg.text, text, MAX_MSG_LENGTH);
    msg.text[MAX_MSG_LENGTH] = '\0';
    msg.rssi = rssi;
    msg.isMine = false;
    msg.isSystem = false;
    addMessage(msg);
}

void ChatUI::addSentMessage(const char* text) {
    ChatMessage msg;
    strncpy(msg.nickname, _nickname, MAX_NICKNAME_LEN);
    msg.nickname[MAX_NICKNAME_LEN] = '\0';
    strncpy(msg.text, text, MAX_MSG_LENGTH);
    msg.text[MAX_MSG_LENGTH] = '\0';
    msg.rssi = 0;
    msg.isMine = true;
    msg.isSystem = false;
    addMessage(msg);
}

void ChatUI::addSystemMessage(const char* text) {
    ChatMessage msg;
    strcpy(msg.nickname, "SYS");
    strncpy(msg.text, text, MAX_MSG_LENGTH);
    msg.text[MAX_MSG_LENGTH] = '\0';
    msg.rssi = 0;
    msg.isMine = false;
    msg.isSystem = true;
    addMessage(msg);
}

void ChatUI::addMessage(const ChatMessage& msg) {
    if (_messageCount >= MAX_MESSAGES) {
        memmove(&_messages[0], &_messages[1], sizeof(ChatMessage) * (MAX_MESSAGES - 1));
        _messageCount = MAX_MESSAGES - 1;
    }
    _messages[_messageCount++] = msg;
    if (_initialized) {
        drawMessages();
    }
}

bool ChatUI::processInput() {
    M5Cardputer.update();

    // Cursor blink
    if (_initialized && millis() - _lastCursorBlink > 500) {
        _cursorVisible = !_cursorVisible;
        _lastCursorBlink = millis();
        drawInput();
    }

    if (!M5Cardputer.Keyboard.isChange()) {
        return false;
    }

    if (!M5Cardputer.Keyboard.isPressed()) {
        return false;
    }

    Keyboard_Class::KeysState keys = M5Cardputer.Keyboard.keysState();

    if (keys.del) {
        if (_inputPos > 0) {
            _inputPos--;
            _inputBuffer[_inputPos] = '\0';
            if (_initialized) drawInput();
        }
        return false;
    }

    if (keys.enter) {
        if (_inputPos > 0) {
            return true;
        }
        return false;
    }

    if (keys.word.size() > 0) {
        char c = keys.word[0];
        if (_inputPos < MAX_MSG_LENGTH - 1 && isPrintable(c)) {
            _inputBuffer[_inputPos++] = c;
            _inputBuffer[_inputPos] = '\0';
            if (_initialized) drawInput();
        }
    }

    return false;
}

void ChatUI::clearInput() {
    memset(_inputBuffer, 0, sizeof(_inputBuffer));
    _inputPos = 0;
    if (_initialized) drawInput();
}

void ChatUI::redraw() {
    if (!_initialized) return;

    LCD.fillScreen(COLOR_BG);
    drawHeader();
    drawMessages();
    drawInput();
}

void ChatUI::drawHeader() {
    if (!_initialized) return;

    LCD.fillRect(0, 0, DISPLAY_WIDTH, HEADER_HEIGHT, COLOR_HEADER_BG);

    // Status indicator
    LCD.fillCircle(6, HEADER_HEIGHT / 2, 4, _connected ? COLOR_STATUS_OK : COLOR_STATUS_ERR);

    // Frequency
    LCD.setTextColor(COLOR_HEADER_TEXT);
    LCD.setTextSize(1);
    char freqStr[16];
    sprintf(freqStr, "%.1fMHz", _frequency);
    LCD.setCursor(16, 3);
    LCD.print(freqStr);

    // Nickname (right side)
    int nickLen = strlen(_nickname) * 6;  // Approximate width
    LCD.setCursor(DISPLAY_WIDTH - nickLen - 4, 3);
    LCD.print(_nickname);
}

void ChatUI::drawMessages() {
    if (!_initialized) return;

    // Clear message area
    LCD.fillRect(0, MSG_AREA_Y, DISPLAY_WIDTH, MSG_AREA_HEIGHT, COLOR_BG);

    LCD.setTextSize(1);

    int y = MSG_AREA_Y + 2;
    int lineHeight = 10;
    int maxY = MSG_AREA_Y + MSG_AREA_HEIGHT - lineHeight;

    // Show last few messages that fit
    size_t startIdx = 0;
    if (_messageCount > 8) {
        startIdx = _messageCount - 8;
    }

    for (size_t i = startIdx; i < _messageCount && y < maxY; i++) {
        ChatMessage& msg = _messages[i];

        // Set color
        uint16_t color;
        if (msg.isSystem) {
            color = COLOR_SYSTEM_MSG;
        } else if (msg.isMine) {
            color = COLOR_MY_MSG;
        } else {
            color = COLOR_OTHER_MSG;
        }

        // Draw nickname line
        LCD.setTextColor(color);
        LCD.setCursor(2, y);
        if (msg.isSystem) {
            LCD.printf("[%s] %s", msg.nickname, msg.text);
        } else {
            LCD.printf("<%s> %s", msg.nickname, msg.text);
        }

        y += lineHeight + 2;
    }
}

void ChatUI::drawInput() {
    if (!_initialized) return;

    // Input area background
    LCD.fillRect(0, INPUT_Y, DISPLAY_WIDTH, INPUT_HEIGHT, COLOR_INPUT_BG);

    // Draw prompt
    LCD.setTextColor(COLOR_MY_MSG);
    LCD.setTextSize(1);
    LCD.setCursor(2, INPUT_Y + 4);
    LCD.print(">");

    // Draw input text
    LCD.setTextColor(COLOR_INPUT_TEXT);
    LCD.setCursor(10, INPUT_Y + 4);

    // Show last part of text if too long
    const char* displayText = _inputBuffer;
    int maxChars = (DISPLAY_WIDTH - 20) / 6;
    int len = strlen(_inputBuffer);
    if (len > maxChars) {
        displayText = _inputBuffer + (len - maxChars);
    }
    LCD.print(displayText);

    // Draw cursor
    if (_cursorVisible) {
        int cursorX = 10 + strlen(displayText) * 6;
        LCD.fillRect(cursorX, INPUT_Y + 2, 2, INPUT_HEIGHT - 4, COLOR_INPUT_TEXT);
    }
}

void ChatUI::scrollToBottom() {
    // Auto-scrolled in drawMessages
}

void ChatUI::drawRSSIBar(int x, int y, int16_t rssi) {
    int bars = 0;
    if (rssi > -70) bars = 4;
    else if (rssi > -85) bars = 3;
    else if (rssi > -100) bars = 2;
    else if (rssi > -115) bars = 1;

    uint16_t color = (bars >= 3) ? COLOR_RSSI_GOOD : (bars >= 2) ? COLOR_RSSI_MED : COLOR_RSSI_BAD;

    for (int i = 0; i < 4; i++) {
        int barHeight = 3 + i * 2;
        int barX = x + i * 5;
        int barY = y + (10 - barHeight);
        if (i < bars) {
            LCD.fillRect(barX, barY, 4, barHeight, color);
        } else {
            LCD.drawRect(barX, barY, 4, barHeight, TFT_DARKGREY);
        }
    }
}

void ChatUI::showRSSI(int16_t rssi) {
    if (!_initialized) return;
    char rssiStr[16];
    sprintf(rssiStr, "%ddBm", rssi);
    LCD.setTextColor(COLOR_HEADER_TEXT);
    LCD.fillRect(100, 0, 50, HEADER_HEIGHT, COLOR_HEADER_BG);
    LCD.setCursor(100, 3);
    LCD.print(rssiStr);
}

int ChatUI::wrapText(const char* text, int maxWidth, String* lines, int maxLines) {
    // Simplified - just return single line
    if (maxLines > 0) {
        lines[0] = text;
        return 1;
    }
    return 0;
}
