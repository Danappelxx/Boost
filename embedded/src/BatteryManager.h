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

    static constexpr float IS_VEHICLE_OFF_THRESHOLD = 13.75f;
    static constexpr float IS_VEHICLE_BATTERY_THRESHOLD = 10.00f;
};
