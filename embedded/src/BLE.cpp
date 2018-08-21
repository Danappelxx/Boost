#include "BLE.h"
#include <assert.h>
#include <sstream>

//MARK: UUID
// 16 bit
BLE::UUID::UUID(uint16_t data) {
    this->data = { LOW_BYTE(data), HIGH_BYTE(data) };
}

// 128 bit
BLE::UUID::UUID(const uint8_t (&data)[16]) {
    this->data = std::vector<uint8_t>(data, data + 16);
}

// converts a single hex char to a number (0 - 15)
// https://github.com/graeme-hill/crossguid/blob/master/src/guid.cpp#L161
uint8_t hexDigitToChar(char ch) {
    // 0-9
    if (ch > 47 && ch < 58)
        return ch - 48;

    // a-f
    if (ch > 96 && ch < 103)
        return ch - 87;

    // A-F
    if (ch > 64 && ch < 71)
        return ch - 55;

    return 0;
}

// 128 bit
BLE::UUID::UUID(const std::string string) {
    std::vector<uint8_t> data;

    // char 1 is upper bytes, char 2 is lower bytes
    char char1;
    char char2;
    bool wantsCharOne = true;
    for (char c : string) {
        if (c == '-')
            continue;

        if (wantsCharOne)
            char1 = c;
        else {
            char2 = c;

            uint8_t combined = hexDigitToChar(char1) * 16 + hexDigitToChar(char2);
            data.push_back(combined);
        }

        wantsCharOne = !wantsCharOne;
    }
    this->data = data;
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

const uint8_t* BLE::UUID::data128() const {
    return data.data();
}

//MARK: Characteristic
BLE::Characteristic::Characteristic(const UUID& type, const Properties& properties): type(type), properties(properties) {
    if (isNotify()) {
        this->clientConfigurationDescriptor = std::make_shared<MutableDescriptor>(
            UUID(GATT_CLIENT_CHARACTERISTICS_CONFIGURATION),
            std::vector<uint8_t>{ GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION, 0 });
    }
    if (isIndicate()) {
        this->clientConfigurationDescriptor = std::make_shared<MutableDescriptor>(
            UUID(GATT_CLIENT_CHARACTERISTICS_CONFIGURATION),
            std::vector<uint8_t>{ GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_INDICATION, 0 });
    }
}

void BLE::Characteristic::addDescriptor(std::shared_ptr<Descriptor> descriptor) {
    descriptors.push_back(descriptor);
    descriptor->characteristic = shared_from_this();
}

void BLE::IndicateCharacteristic::sendIndicate() {
    if (handle == -1) {
        Serial.println("Characteristic has not been added! Cannot send indicate");
        return;
    }

    const std::vector<uint8_t>& value = getValue();
    // btstack makes a copy, even though it isn't marked as such
    ble.sendIndicate(handle, const_cast<uint8_t*>(value.data()), value.size());
}

void BLE::NotifyCharacteristic::sendNotify() {
    if (handle == -1) {
        Serial.println("Characteristic has not been added! Cannot send notify");
        return;
    }

    const std::vector<uint8_t>& value = getValue();
    // btstack makes a copy, even though it isn't marked as such
    ble.sendNotify(handle, const_cast<uint8_t*>(value.data()), value.size());
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
BLE::Manager::Manager(): connected(false) {
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

void BLE::Manager::addService(std::shared_ptr<Service> service) {
    services.push_back(service);

    if (service->getType().is16()) {
        ble.addService(service->getType().data16());
    } else {
        ble.addService(const_cast<uint8_t*>(service->getType().data128()));
    }

    for (const std::shared_ptr<Characteristic>& characteristic : service->getCharacteristics()) {
        // add the characteristic
        uint16_t handle;
        if (characteristic->getType().is16()) {
            handle = ble.addCharacteristic(
                characteristic->getType().data16(),
                static_cast<uint16_t>(characteristic->getProperties()),
                const_cast<uint8_t*>(characteristic->getValue().data()),
                characteristic->getValue().size());
        } else {
            handle = ble.addCharacteristic(
                const_cast<uint8_t*>(characteristic->getType().data128()),
                static_cast<uint16_t>(characteristic->getProperties()),
                const_cast<uint8_t*>(characteristic->getValue().data()),
                characteristic->getValue().size());
        }

        characteristicHandles[handle] = characteristic;
        characteristic->handle = handle;
        Serial.printlnf("Added characteristic handle: %d", handle);

        // add the other descriptors
        for (const std::shared_ptr<Descriptor>& descriptor : characteristic->getDescriptors()) {
            uint16_t handle = 0;
//            if (descriptor->getType().is16()) {
//                handle = ble.addDescriptor(
//                    descriptor->getType().data16(),
//                    static_cast<uint16_t>(descriptor->getProperties()),
//                    const_cast<uint8_t*>(descriptor->getValue().data()),
//                    descriptor->getValue().size());
//            } else {
//                handle = ble.addDescriptor(
//                    descriptor->getType().data128(),
//                    static_cast<uint16_t>(descriptor->getProperties()),
//                    const_cast<uint8_t*>(descriptor->getValue().data()),
//                    descriptor->getValue().size());
//            }
//
//            descriptorHandles[handle] = descriptor;
            Serial.printlnf("Added descriptor handle: %d", handle);
        }

        // client characteristic configuration descriptor
        if (std::shared_ptr<Descriptor> descriptor = characteristic->clientConfigurationDescriptor) {
            descriptorHandles[handle + 1] = descriptor;
            Serial.printlnf("Added client characteristic configuration descriptor handle: %d", handle + 1);
        }
    }
}

uint16_t BLE::Manager::onReadCallback(uint16_t handle, uint8_t* buffer, uint16_t bufferSize) {
    std::shared_ptr<Characteristic> characteristic = characteristicHandles[handle];
    if (characteristic) {
        const std::vector<uint8_t>& value = characteristic->getValue();
        // if buffer is null, don't copy and just return size
        if (buffer)
            memcpy(buffer, value.data(), bufferSize);
        Serial.printlnf("Read characteristic, handle: %d, size: %d", handle, value.size());
        return value.size();
    }

    std::shared_ptr<Descriptor> descriptor = descriptorHandles[handle];
    if (descriptor) {
        const std::vector<uint8_t>& value = descriptor->getValue();
        // if buffer is null, don't copy and just return size
        if (buffer)
            memcpy(buffer, value.data(), bufferSize);
        Serial.printlnf("Read characteristic, handle: %d, size: %d", handle, value.size());
        return value.size();
    }

    Serial.println("Could not find matching characteristic or descriptor for read!");
    return 0;
}

int BLE::Manager::onWriteCallback(uint16_t handle, uint8_t* buffer, uint16_t bufferSize) {
    std::vector<uint8_t> newValue(buffer, buffer + bufferSize);

    std::shared_ptr<Characteristic> characteristic = characteristicHandles[handle];
    if (characteristic) {
        uint8_t ret = static_cast<uint8_t>(characteristic->setValue(newValue));
        Serial.printlnf("Wrote characteristic, handle: %d, code: %d", handle, ret);
        return ret;
    }

    std::shared_ptr<Descriptor> descriptor = descriptorHandles[handle];
    if (descriptor) {
        uint8_t ret = static_cast<uint8_t>(descriptor->setValue(newValue));
        Serial.printlnf("Wrote descriptor, handle: %d, code: %d", handle, ret);
        return ret;
    }

    Serial.printlnf("Could not find matching characteristic for write! Handle: %d", handle);
    return static_cast<uint8_t>(Error::UnlikelyError);
}

void BLE::Manager::onConnectedCallback(BLEStatus_t status, uint16_t handle) {
    switch (status) {
        case BLE_STATUS_OK:
            Serial.printlnf("Successfully connected to device! Handle: %d", handle);

            connected = true;

            if (std::shared_ptr<IndicateCharacteristic> serviceChangedCharacteristic = this->serviceChangedCharacteristic) {
                serviceChangedCharacteristic->sendIndicate();
            }

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
    connected = false;
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
