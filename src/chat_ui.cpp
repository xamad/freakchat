/**
 * Chat UI implementation for M5Stack Cardputer
 */

#include "chat_ui.h"

// Global instance
ChatUI chatUI;

ChatUI::ChatUI()
    : _inputPos(0)
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
    strcpy(_nickname, "User");  // Default nickname
}

void ChatUI::begin() {
    // Create sprite for double buffering
    _canvas.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    _canvas.setTextDatum(TL_DATUM);

    // Initial draw
    redraw();
}

void ChatUI::setNickname(const char* nickname) {
    strncpy(_nickname, nickname, MAX_NICKNAME_LEN);
    _nickname[MAX_NICKNAME_LEN] = '\0';
    drawHeader();
}

void ChatUI::updateStatus(bool connected, float frequency) {
    if (_connected != connected || _frequency != frequency) {
        _connected = connected;
        _frequency = frequency;
        drawHeader();
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
    // Shift messages if buffer is full
    if (_messageCount >= MAX_MESSAGES) {
        memmove(&_messages[0], &_messages[1], sizeof(ChatMessage) * (MAX_MESSAGES - 1));
        _messageCount = MAX_MESSAGES - 1;
    }

    _messages[_messageCount++] = msg;
    scrollToBottom();
    drawMessages();
}

bool ChatUI::processInput() {
    M5Cardputer.update();

    // Cursor blink
    if (millis() - _lastCursorBlink > 500) {
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

    // Handle special keys
    if (keys.del) {
        // Backspace
        if (_inputPos > 0) {
            _inputPos--;
            _inputBuffer[_inputPos] = '\0';
            drawInput();
        }
        return false;
    }

    if (keys.enter) {
        // Enter - send message
        if (_inputPos > 0) {
            return true;
        }
        return false;
    }

    // Handle Fn+key combinations for special characters
    // The Cardputer keyboard provides these through the keys.word string

    // Regular character input
    if (keys.word.size() > 0) {
        char c = keys.word[0];
        if (_inputPos < MAX_MSG_LENGTH - 1 && isPrintable(c)) {
            _inputBuffer[_inputPos++] = c;
            _inputBuffer[_inputPos] = '\0';
            drawInput();
        }
    }

    return false;
}

void ChatUI::clearInput() {
    memset(_inputBuffer, 0, sizeof(_inputBuffer));
    _inputPos = 0;
    drawInput();
}

void ChatUI::redraw() {
    _canvas.fillSprite(COLOR_BG);
    drawHeader();
    drawMessages();
    drawInput();
    _canvas.pushSprite(0, 0);
}

void ChatUI::drawHeader() {
    // Header background
    _canvas.fillRect(0, 0, DISPLAY_WIDTH, HEADER_HEIGHT, COLOR_HEADER_BG);

    // Status indicator
    int statusX = 2;
    _canvas.fillCircle(statusX + 4, HEADER_HEIGHT / 2, 4, _connected ? COLOR_STATUS_OK : COLOR_STATUS_ERR);

    // Frequency
    _canvas.setTextColor(COLOR_HEADER_TEXT);
    _canvas.setTextSize(1);
    char freqStr[16];
    sprintf(freqStr, "%.1fMHz", _frequency);
    _canvas.drawString(freqStr, statusX + 14, 3);

    // Nickname (right aligned)
    int nickWidth = _canvas.textWidth(_nickname);
    _canvas.drawString(_nickname, DISPLAY_WIDTH - nickWidth - 4, 3);

    // Push header update
    _canvas.pushSprite(0, 0);
}

void ChatUI::drawMessages() {
    // Clear message area
    _canvas.fillRect(0, MSG_AREA_Y, DISPLAY_WIDTH, MSG_AREA_HEIGHT, COLOR_BG);

    _canvas.setTextSize(MSG_FONT_SIZE);

    int y = MSG_AREA_Y + 2;
    int lineHeight = 10;
    int maxY = MSG_AREA_Y + MSG_AREA_HEIGHT - lineHeight;

    // Calculate how many messages we can show
    int visibleLines = MSG_AREA_HEIGHT / lineHeight;

    // Start from scroll position
    size_t startIdx = 0;
    if (_messageCount > 0) {
        // Try to show recent messages
        int linesNeeded = 0;
        for (int i = _messageCount - 1; i >= 0 && linesNeeded < visibleLines; i--) {
            // Each message takes at least 1 line for nickname + lines for text
            String lines[5];
            int numLines = wrapText(_messages[i].text, DISPLAY_WIDTH - 10, lines, 5);
            linesNeeded += 1 + numLines;  // nickname line + text lines
            if (linesNeeded <= visibleLines) {
                startIdx = i;
            }
        }
    }

    // Draw messages
    for (size_t i = startIdx; i < _messageCount && y < maxY; i++) {
        ChatMessage& msg = _messages[i];

        // Choose color based on message type
        uint16_t nickColor, textColor;
        if (msg.isSystem) {
            nickColor = COLOR_SYSTEM_MSG;
            textColor = COLOR_SYSTEM_MSG;
        } else if (msg.isMine) {
            nickColor = COLOR_MY_MSG;
            textColor = TFT_WHITE;
        } else {
            nickColor = COLOR_OTHER_MSG;
            textColor = TFT_WHITE;
        }

        // Draw nickname
        _canvas.setTextColor(nickColor);
        char nickLine[MAX_NICKNAME_LEN + 10];
        if (msg.isSystem) {
            sprintf(nickLine, "[%s]", msg.nickname);
        } else {
            sprintf(nickLine, "<%s>", msg.nickname);
        }
        _canvas.drawString(nickLine, 2, y);

        // Draw RSSI if available (for received messages)
        if (!msg.isMine && !msg.isSystem && msg.rssi != 0) {
            drawRSSIBar(DISPLAY_WIDTH - 25, y, msg.rssi);
        }

        y += lineHeight;

        // Draw wrapped message text
        String lines[5];
        int numLines = wrapText(msg.text, DISPLAY_WIDTH - 10, lines, 5);
        _canvas.setTextColor(textColor);

        for (int j = 0; j < numLines && y < maxY; j++) {
            _canvas.drawString(lines[j], 6, y);
            y += lineHeight;
        }

        y += 2;  // Space between messages
    }

    _canvas.pushSprite(0, 0);
}

void ChatUI::drawInput() {
    // Input area background
    _canvas.fillRect(0, INPUT_Y, DISPLAY_WIDTH, INPUT_HEIGHT, COLOR_INPUT_BG);

    // Draw prompt
    _canvas.setTextColor(COLOR_MY_MSG);
    _canvas.setTextSize(1);
    _canvas.drawString(">", 2, INPUT_Y + 3);

    // Draw input text
    _canvas.setTextColor(COLOR_INPUT_TEXT);
    int textX = 10;
    int maxTextWidth = DISPLAY_WIDTH - 20;

    // Show end of text if it's too long
    String displayText = _inputBuffer;
    int textWidth = _canvas.textWidth(displayText);
    if (textWidth > maxTextWidth) {
        // Scroll to show cursor position
        while (_canvas.textWidth(displayText) > maxTextWidth && displayText.length() > 0) {
            displayText = displayText.substring(1);
        }
    }

    _canvas.drawString(displayText, textX, INPUT_Y + 3);

    // Draw cursor
    if (_cursorVisible) {
        int cursorX = textX + _canvas.textWidth(displayText);
        _canvas.fillRect(cursorX, INPUT_Y + 2, 2, INPUT_HEIGHT - 4, COLOR_INPUT_TEXT);
    }

    _canvas.pushSprite(0, 0);
}

void ChatUI::scrollToBottom() {
    // Calculate scroll position to show latest messages
    // This is simplified - messages are auto-scrolled in drawMessages
}

void ChatUI::drawRSSIBar(int x, int y, int16_t rssi) {
    // Draw 4 bars for RSSI indication
    // RSSI typically ranges from -120 (weak) to -30 (strong)
    int bars = 0;
    if (rssi > -70) bars = 4;
    else if (rssi > -85) bars = 3;
    else if (rssi > -100) bars = 2;
    else if (rssi > -115) bars = 1;

    uint16_t color = COLOR_RSSI_BAD;
    if (bars >= 3) color = COLOR_RSSI_GOOD;
    else if (bars >= 2) color = COLOR_RSSI_MED;

    for (int i = 0; i < 4; i++) {
        int barHeight = 3 + i * 2;
        int barX = x + i * 5;
        int barY = y + (10 - barHeight);
        if (i < bars) {
            _canvas.fillRect(barX, barY, 4, barHeight, color);
        } else {
            _canvas.drawRect(barX, barY, 4, barHeight, TFT_DARKGREY);
        }
    }
}

void ChatUI::showRSSI(int16_t rssi) {
    // Show RSSI in header briefly
    char rssiStr[16];
    sprintf(rssiStr, "%ddBm", rssi);
    _canvas.setTextColor(COLOR_HEADER_TEXT);
    _canvas.fillRect(100, 0, 50, HEADER_HEIGHT, COLOR_HEADER_BG);
    _canvas.drawString(rssiStr, 100, 3);
    _canvas.pushSprite(0, 0);
}

int ChatUI::wrapText(const char* text, int maxWidth, String* lines, int maxLines) {
    String input = text;
    int lineCount = 0;

    while (input.length() > 0 && lineCount < maxLines) {
        if (_canvas.textWidth(input) <= maxWidth) {
            lines[lineCount++] = input;
            break;
        }

        // Find break point
        int breakPoint = input.length();
        for (int i = input.length() - 1; i > 0; i--) {
            String test = input.substring(0, i);
            if (_canvas.textWidth(test) <= maxWidth) {
                // Try to break at space
                int spacePos = test.lastIndexOf(' ');
                if (spacePos > 0 && spacePos > i - 10) {
                    breakPoint = spacePos;
                } else {
                    breakPoint = i;
                }
                break;
            }
        }

        lines[lineCount++] = input.substring(0, breakPoint);
        input = input.substring(breakPoint);
        if (input.startsWith(" ")) {
            input = input.substring(1);
        }
    }

    return lineCount;
}
