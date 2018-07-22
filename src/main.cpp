/* Vortex CAN messenger
 * Exposes vehicle CAN interfaces over Bluetooth Low Energy
 */

#define PLATFORM_ID 88

#include "application.h"
#include "carloop.h"
#include "SLCAN.h"

SYSTEM_THREAD(ENABLED);
BLE_SETUP(DISABLED);

CANChannel can(CAN_D1_D2);
SLCAN slcan(can);
float battery;

void disableCarloop() {
    // https://github.com/carloop/carloop-library/blob/186acf9403c375e9e251769fb4669c9a4d226704/src/carloop.cpp#L82
    pinMode(CarloopRevision2::CAN_ENABLE_PIN, OUTPUT);
    digitalWrite(CarloopRevision2::CAN_ENABLE_PIN, CarloopRevision2::CAN_ENABLE_INACTIVE);
}

void enableBattery() {
    pinMode(CarloopRevision2::BATTERY_PIN, INPUT);
}

float readBattery() {
    static constexpr auto MAX_ANALOG_VALUE = 4096;
    static constexpr auto MAX_ANALOG_VOLTAGE = 3.3f;
    auto adcValue = analogRead(CarloopRevision2::BATTERY_PIN);
    return adcValue * MAX_ANALOG_VOLTAGE / MAX_ANALOG_VALUE * CarloopRevision2::BATTERY_FACTOR;
}

void receiveMessages() {
    CANMessage message;
    while(can.receive(message)) {
        slcan.printReceivedMessage(message);
    }
}

void serialEvent() {
    slcan.parseInput(Serial.read());
}

void setup() {
    Serial.begin(9600);

    // disable carloop's high speed CAN to conserve power since we're not using it
    disableCarloop();
    enableBattery();
}

void loop() {
    battery = readBattery();
    receiveMessages();
}
