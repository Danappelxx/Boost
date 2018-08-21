#include "SLCAN.h"

void SLCAN::transmitMessage(const char *buf, unsigned n) {
    if (!can.isEnabled())
        return;

    if (n < 4)
        return;

    CANMessage message;
    message.id = (hex2int(buf[0]) << 8) | (hex2int(buf[1]) << 4) | hex2int(buf[2]);
    message.len = hex2int(buf[3]);
    buf += 4;
    n -= 4;

    if (message.len > 8)
        return;

    for (unsigned i = 0; i < message.len && n >= 2; i++, buf += 2, n -= 2) {
        message.data[i] = (hex2int(buf[0]) << 4) | hex2int(buf[1]);
    }

    can.transmit(message);
}

void SLCAN::printReceivedMessage(const CANMessage &message) {
    Serial.printf("t%03x%d", message.id, message.len);
    for(auto i = 0; i < message.len; i++) {
        Serial.printf("%02x", message.data[i]);
    }
    Serial.write(SLCAN::NEW_LINE);
}

void SLCAN::parseInput(char c) {
    if (inputPos < sizeof(inputBuffer)) {
        inputBuffer[inputPos] = c;
        inputPos++;
    }

    if (c == NEW_LINE) {
        switch (inputBuffer[0]) {
            case 'O':
                openCAN();
                break;
            case 'C':
                closeCAN();
                break;
            case 't':
                transmitMessage(&inputBuffer[1], inputPos - 2);
                break;
        }
        inputPos = 0;
    }
}

unsigned SLCAN::hex2int(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    else if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return 0;
}

void SLCAN::openCAN() {
    if (can.isEnabled())
        return;

    can.begin(33333);
}

void SLCAN::closeCAN() {
    if (!can.isEnabled())
        return;

    can.end();
}
