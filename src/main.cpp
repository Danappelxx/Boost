/* Vortex CAN messenger
 * Exposes vehicle CAN interfaces over Bluetooth Low Energy
 */

#define PLATFORM_ID 88

#include "application.h"
#include "carloop.h"
#include "SLCAN.h"
#include "VortexBluetooth.h"

SYSTEM_THREAD(ENABLED);
BLE_SETUP(DISABLED);

void disableCarloop();
void enableBattery();
float readBattery();
void receiveMessages();

class LEDBlinkerService: public BLE::Service {
    class BlinkCharacteristic: public BLE::MutableCharacteristic {
        enum class State: uint8_t {
            Off = 0,
            On = 1
        };
    public:

        BlinkCharacteristic(): MutableCharacteristic(
            BLE::UUID("6962CDC6-DCB1-465B-8AA4-23491CAF4840"),
            { static_cast<uint8_t>(State::Off) }) {
            pinMode(D7, OUTPUT);
        }

        virtual BLE::Error setValue(const std::vector<uint8_t>& newValue) override {
            auto ret = MutableCharacteristic::setValue(newValue);
            updateLed(getState());
            return ret;
        }

        State getState() {
            return static_cast<State>(getValue()[0]);
        }
        void updateLed(State state) {
            digitalWrite(D7, static_cast<uint8_t>(state));
        }
    };

public:
    LEDBlinkerService():
        Service(BLE::UUID("70DA7AB7-4FE2-4614-B092-2E8EC60290BB")) {
        addCharacteristic(std::make_shared<BlinkCharacteristic>());
    }
};

CANChannel can(CAN_D1_D2);
SLCAN slcan(can);
float battery;
std::unique_ptr<BLE::Manager> bluetooth;

void setup() {
    Serial.begin();
    delay(5000);

    // disable carloop's high speed CAN to conserve power since we're not using it
    //disableCarloop();
    //enableBattery();
    Serial.println("About to init bluetooth");
    bluetooth = BLE::vortexBluetooth();
    Serial.println("Initialized bluetooth!");

    bluetooth->addService(std::make_shared<LEDBlinkerService>());

    Serial.println("About to begin advertising");
    bluetooth->startAdvertising();
    Serial.println("Began advertising!");
}

void loop() {
    //battery = readBattery();
    //receiveMessages();
}

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
