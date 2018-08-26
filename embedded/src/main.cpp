/* Boost CAN messenger
 * Exposes vehicle CAN interfaces over Bluetooth Low Energy
 */

#define PLATFORM_ID 88

#include "application.h"
#include "carloop.h"
#include "SLCAN.h"
#include "Bluetooth.h"

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(MANUAL);
BLE_SETUP(DISABLED);

void disableCarloop();
void enableBattery();
float readBattery();
void receiveMessages();

class LEDBlinkerService: public BLE::Service {
public:
    class BlinkCharacteristic: public BLE::MutableCharacteristic {
    public:
        enum class State: uint8_t {
            Off = 0,
            On = 1
        };

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

    LEDBlinkerService()
        : Service(BLE::UUID("70DA7AB7-4FE2-4614-B092-2E8EC60290BB")) {
        blinkCharacteristic = std::make_shared<BlinkCharacteristic>();

        addCharacteristic(blinkCharacteristic);
    }

    std::shared_ptr<BlinkCharacteristic> blinkCharacteristic;
};

class CANService: public BLE::Service {
    class SteeringWheelCharacteristic: public BLE::IndicateCharacteristic {
    public:
        SteeringWheelCharacteristic()
            : IndicateCharacteristic(BLE::UUID("BBBEF1D2-E1E6-4189-BE0B-C00D6D3CC6BB"), { 0 }) {}

        void newState(uint8_t state) {
            setValue({ state });
        }
    };

    class BatteryCharacteristic: public BLE::IndicateCharacteristic {
    public:
        BatteryCharacteristic()
            : IndicateCharacteristic(BLE::UUID("63F13CE9-63B0-4ED3-8EBA-27441DDFC18E"), { 0, 0 }) {}

        void newState(float state) {
            // two decimals of precision
            uint16_t value = (uint16_t)(state * 100);
            // split up uint16_t into two
            setValue({ (uint8_t)(value & 0xff), (uint8_t)(value >> 8) });
            Serial.printlnf("Sent battery value notification: %.2f", state);
        }
    };

public:
    CANService() : Service(BLE::UUID("9A7E8B1D-EA49-40D6-B575-406AD07F8816")) {
        steeringWheelCharacteristic = std::make_shared<SteeringWheelCharacteristic>();
        batteryCharacteristic = std::make_shared<BatteryCharacteristic>();

        addCharacteristic(steeringWheelCharacteristic);
        addCharacteristic(batteryCharacteristic);
    }

    std::shared_ptr<SteeringWheelCharacteristic> steeringWheelCharacteristic;
    std::shared_ptr<BatteryCharacteristic> batteryCharacteristic;
};

CANChannel can(CAN_D1_D2);
std::unique_ptr<BLE::Manager> bluetooth;
std::shared_ptr<CANService> canService;
std::shared_ptr<LEDBlinkerService> ledBlinkerService;

void setup() {
    Serial.begin();

    pinMode(D7, OUTPUT);

    digitalWrite(D7, HIGH);
    delay(1000);
    digitalWrite(D7, LOW);
    delay(1000);
    digitalWrite(D7, HIGH);

    // disable carloop's high speed CAN to conserve power since we're not using it
    disableCarloop();
    enableBattery();

    Serial.println("About to init bluetooth");
    bluetooth = BLE::bluetooth();
    Serial.println("Initialized bluetooth!");

    digitalWrite(D7, LOW);

    ledBlinkerService = std::make_shared<LEDBlinkerService>();
    bluetooth->addService(ledBlinkerService);

    canService = std::make_shared<CANService>();
    bluetooth->addService(canService);

    Serial.println("About to begin advertising");
    bluetooth->startAdvertising();
    Serial.println("Began advertising!");

    can.begin(33333);
}

unsigned long timeLastSentBattery = 0;
void loop() {

    if (!bluetooth->isConnected()) {
        Serial.println("Not connected");
        digitalWrite(D7, HIGH);
        delay(750);
        digitalWrite(D7, LOW);
        delay(750);
        return;
    }

    if (millis() - timeLastSentBattery > 500) {
        canService->batteryCharacteristic->newState(readBattery());
        timeLastSentBattery = millis();
    }

    //receiveMessages();

    //TODO: make a lil framework out of this
    CANMessage message;
    while(can.receive(message)) {
        // steering wheel id
        if (message.id == 0x290) {
            // steering wheel button is fourth byte
            uint8_t value = message.data[3];
            canService->steeringWheelCharacteristic->newState(value);
        }
    }
}

void printMessage(const CANMessage& message) {
    Serial.printlnf("length is %d, size is %d", message.len, message.size);
    Serial.printlnf(
        "%x %x %x %x %x %x %x %x",
        message.data[0],
        message.data[1],
        message.data[2],
        message.data[3],
        message.data[4],
        message.data[5],
        message.data[6],
        message.data[7]);
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
