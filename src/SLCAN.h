#include "application.h"

class SLCAN {
public:
    SLCAN(CANChannel& can): can(can) {}

    void transmitMessage(const char *buf, unsigned n);
    void printReceivedMessage(const CANMessage &message);
    void parseInput(char c);
    void openCAN();
    void closeCAN();
private:
    unsigned hex2int(char c);

    CANChannel& can;
    const char NEW_LINE = '\r';
    char inputBuffer[40];
    unsigned inputPos = 0;
};
