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

bool BLE::UUID::is16() const {
    return data.size() == 2;
}

bool BLE::UUID::is128() const {
    // lol
    return !is16();
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
BLE::Characteristic::Characteristic(
    const UUID& type, Properties properties, const std::vector<uint8_t>& value, const std::vector<std::shared_ptr<Descriptor> >& descriptors )
    : type(type), properties(properties), value(value), descriptors(descriptors) {
}

//MARK: Service
BLE::Service::Service(UUID type) : type(type) {
}

void BLE::Service::addCharacteristic(std::shared_ptr<Characteristic> characteristic) {
    characteristics.push_back(characteristic);
}

//MARK: Callback hacks
static BLE::Manager* _manager = nullptr;
static uint16_t (BLE::Manager::*_onReadCallback)(uint16_t, uint8_t*, uint16_t) = nullptr;
static int (BLE::Manager::*_onWriteCallback)(uint16_t, uint8_t*, uint16_t) = nullptr;
static void (BLE::Manager::*_onConnectedCallback)(BLEStatus_t, uint16_t) = nullptr;
static void (BLE::Manager::*_onDisconnectedCallback)(uint16_t) = nullptr;

uint16_t _onReadCallbackWrapper(uint16_t handle, uint8_t* buffer, uint16_t bufferSize) {
    return (_manager->*_onReadCallback)(handle, buffer, bufferSize);
}

int _onWriteCallbackWrapper(uint16_t handle, uint8_t* buffer, uint16_t bufferSize) {
    return (_manager->*_onWriteCallback)(handle, buffer, bufferSize);
}

void _onConnectedCallbackWrapper(BLEStatus_t status, uint16_t handle) {
    return (_manager->*_onConnectedCallback)(status, handle);
}

void _onDisconnectedCallbackWrapper(uint16_t handle) {
    return (_manager->*_onDisconnectedCallback)(handle);
}

void _registerCallbacks(
    BLE::Manager* manager,
    uint16_t (BLE::Manager::*onReadCallback)(uint16_t, uint8_t*, uint16_t),
    int (BLE::Manager::*onWriteCallback)(uint16_t, uint8_t*, uint16_t),
    void (BLE::Manager::*onConnectedCallback)(BLEStatus_t, uint16_t),
    void (BLE::Manager::*onDisconnectedCallback)(uint16_t)) {

    _manager = manager;
    _onReadCallback = onReadCallback;
    _onWriteCallback = onWriteCallback;
    _onConnectedCallback = onConnectedCallback;
    _onDisconnectedCallback = onDisconnectedCallback;

    ble.onDataReadCallback(_onReadCallbackWrapper);
    ble.onDataWriteCallback(_onWriteCallbackWrapper);
    ble.onConnectedCallback(_onConnectedCallbackWrapper);
    ble.onDisconnectedCallback(_onDisconnectedCallbackWrapper);
}

//MARK: Manager
BLE::Manager::Manager() {
    ble.debugLogger(true);
    ble.debugError(true);
    //ble.enablePacketLogger();

    ble.init();

    _registerCallbacks(
       this,
       &BLE::Manager::onReadCallback,
       &BLE::Manager::onWriteCallback,
       &BLE::Manager::onConnectedCallback,
       &BLE::Manager::onDisconnectedCallback);
}

BLE::Manager::~Manager() {
    ble.deInit();
}

void BLE::Manager::addService(Service service) {
    services.push_back(service);

    if (service.getType().is16()) {
        ble.addService(service.getType().data16());
    } else {
        ble.addService(service.getType().data128());
    }

    //TODO: store handles for callbacks
    for (std::shared_ptr<Characteristic> characteristic : service.getCharacteristics()) {
        uint16_t handle = characteristic->_addCharacteristic();
        Serial.printlnf("Added handle: %d", handle);
        characteristicHandles[handle] = characteristic;
    }
}

uint16_t BLE::Manager::onReadCallback(uint16_t handle, uint8_t* buffer, uint16_t bufferSize) {
    std::shared_ptr<Characteristic> characteristic = characteristicHandles[handle];
    if (characteristic) {
        std::vector<uint8_t> value = characteristic->onRead(bufferSize);
        memcpy(buffer, value.data(), value.size());
        return value.size();
    }

    std::shared_ptr<Descriptor> descriptor = descriptorHandles[handle];
    if (descriptor) {
        Serial.println("Found matching characteristic");
        const std::vector<uint8_t>& value = descriptor->getValue();
        memcpy(buffer, value.data(), value.size());
        return value.size();
    }

    Serial.println("Could not find matching characteristic or descriptor for read!");
    return 0;
}

int BLE::Manager::onWriteCallback(uint16_t handle, uint8_t* buffer, uint16_t bufferSize) {
    std::shared_ptr<Characteristic> characteristic = characteristicHandles[handle];
    if (!characteristic) {
        // If characteristic contains client characteristic configuration, then client
        // characteristic configration handle is value_handle+1. Now can't add user_descriptor.
        characteristic = characteristicHandles[handle + 1];

        if (!characteristic) {
            // fooook
            Serial.printlnf("Could not find matching characteristic for write! Handle: %d", handle);
            return static_cast<uint8_t>(Characteristic::Error::UnlikelyError);
        }
    }

    std::vector<uint8_t> newValue(buffer, buffer + bufferSize);
    return static_cast<uint8_t>(characteristic->onWrite(newValue));
}

void BLE::Manager::onConnectedCallback(BLEStatus_t status, uint16_t handle) {
    switch (status) {
        case BLE_STATUS_OK:
            Serial.printlnf("Successfully connected to device! Handle: %d", handle);
            break;
        case BLE_STATUS_DONE:
            Serial.printlnf("Connection done. Handle: %d", handle);
            break;
        case BLE_STATUS_CONNECTION_TIMEOUT:
            Serial.printlnf("Connection timed out. Handle: %d", handle);
            break;
        case BLE_STATUS_CONNECTION_ERROR:
            Serial.printlnf("Connection error. Handle: %d", handle);
            break;
        case BLE_STATUS_OTHER_ERROR:
            Serial.printlnf("Connection other error. Handle: %d", handle);
            break;
    }
}

void BLE::Manager::onDisconnectedCallback(uint16_t handle) {
    Serial.printlnf("Device disconnected. Handle: %d", handle);
}

void BLE::Manager::setAdvertisingParameters(advParams_t* advertisingParameters) {
    ble.setAdvertisementParams(advertisingParameters);
}

void BLE::Manager::setAdvertisementData(std::vector<uint8_t>& advertisementData) {
    ble.setAdvertisementData(advertisementData.size(), advertisementData.data());
}

void BLE::Manager::setScanResponseData(std::vector<uint8_t>& scanResponseData) {
    ble.setScanResponseData(scanResponseData.size(), scanResponseData.data());
}

void BLE::Manager::startAdvertising() {
    ble.startAdvertising();
}
