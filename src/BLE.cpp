#include "BLE.h"

//MARK: UUID
BLE::UUID::UUID(uint16_t data) {
    this->data = { LOW_BYTE(data), HIGH_BYTE(data) };
}

BLE::UUID::UUID(const uint8_t (&data)[16]) {
    this->data = std::vector<uint8_t>(data, data + 16);
}

BLE::UUID::UUID(const std::string string) {
    const uint8_t* data = reinterpret_cast<const uint8_t*>(string.c_str());
    this->data = std::vector<uint8_t>(data, data + 16);
}

uint16_t BLE::UUID::data16() const {
    uint8_t byte1 = data[0];
    uint8_t byte2 = data[1];
    return (byte2 << 8) | byte1;
}

uint8_t* BLE::UUID::data128() const {
    return data.data();
}

//MARK: Characteristic
BLE::Characteristic::Characteristic(UUID type, CharacteristicProperties properties, std::vector<uint8_t> value)
    : type(type), properties(properties), value(value) {
}

//MARK: Service
BLE::Service::Service(UUID type) : type(type) {

}

void BLE::Service::addCharacteristic(Characteristic characteristic) {
    characteristics.push_back(characteristic);
}

//MARK: Manager
BLE::Manager::Manager() {
    ble.debugLogger(true);
    ble.debugError(true);
    ble.enablePacketLogger();

    ble.init();

    //TODO:
    //ble.onConnectedCallback(deviceConnectedCallback);
    //ble.onDisconnectedCallback(deviceDisconnectedCallback);
    //ble.onDataReadCallback(gattReadCallback);
    //ble.onDataWriteCallback(gattWriteCallback);
}

BLE::Manager::~Manager() {
    ble.deInit();
}

void BLE::Manager::addService(Service service) {
    services.push_back(service);

    if (service.getType().data.size() == 2) {
        ble.addService(service.getType().data16());
    } else {
        ble.addService(service.getType().data128());
    }

    //TODO: store handles for callbacks
    for (const Characteristic& characteristic : service.getCharacteristics()) {
        if (characteristic.getType().data.size() == 2) {
            ble.addCharacteristicDynamic(
                characteristic.getType().data16(),
                characteristic.getProperties(),
                const_cast<uint8_t*>(characteristic.getValue().data()),
                characteristic.getValue().size());
        } else {
            ble.addCharacteristicDynamic(
                characteristic.getType().data128(),
                characteristic.getProperties(),
                const_cast<uint8_t*>(characteristic.getValue().data()),
                characteristic.getValue().size());
        }
    }
}
