#include "BatteryManager.h"
#include "carloop.h"

void BatteryManager::setup() {
    // disable carloop's high speed CAN to conserve power since we're not using it
    disableCarloop();
    enableBatteryReadings();

    // right off the bat - if the vehicle is off then go back to sleep.
    sleepIfLowBattery();
}

void BatteryManager::disableCarloop() {
    // https://github.com/carloop/carloop-library/blob/186acf9403c375e9e251769fb4669c9a4d226704/src/carloop.cpp#L82
    pinMode(CarloopRevision2::CAN_ENABLE_PIN, OUTPUT);
    digitalWrite(CarloopRevision2::CAN_ENABLE_PIN, CarloopRevision2::CAN_ENABLE_INACTIVE);
}

void BatteryManager::enableBatteryReadings() {
    pinMode(CarloopRevision2::BATTERY_PIN, INPUT);
}

float BatteryManager::readBattery() {
    static constexpr auto MAX_ANALOG_VALUE = 4096;
    static constexpr auto MAX_ANALOG_VOLTAGE = 3.3f;
    auto adcValue = analogRead(CarloopRevision2::BATTERY_PIN);
    return adcValue * MAX_ANALOG_VOLTAGE / MAX_ANALOG_VALUE * CarloopRevision2::BATTERY_FACTOR;
}

void BatteryManager::sleepIfLowBattery() {
    float battery = readBattery();
    if (battery < SLEEP_THRESHOLD && battery > MIN_VEHICLE_BATTERY) {
        // sleep indefinitely (seconds=0)
        // wake up when a rising edge signal is applied to the WKP pin or the reset button is pressed
        System.sleep(SLEEP_MODE_DEEP, 0);
        // will never wake up here... but just in case
        System.reset();
    }
}
