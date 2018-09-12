#pragma once

class BatteryManager {
public:
    BatteryManager() {}

    void setup();
    float readBattery();
    // puts the device to sleep if vehicle is off
    void sleepIfLowBattery();

private:
    void disableCarloop();
    void enableBatteryReadings();

    static constexpr float SLEEP_THRESHOLD = 14.00f;
    static constexpr float MIN_VEHICLE_BATTERY = 10.00f;
};
