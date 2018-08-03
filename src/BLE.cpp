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

const uint8_t* BLE::UUID::data128() const {
    return data.data();
}

//MARK: Characteristic
void BLE::Characteristic::addDescriptor(std::shared_ptr<Descriptor> descriptor) {
    descriptors.push_back(descriptor);
    descriptor->characteristic = shared_from_this();
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

    for (const std::shared_ptr<Characteristic>& characteristic : service.getCharacteristics()) {
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
                characteristic->getType().data128(),
                static_cast<uint16_t>(characteristic->getProperties()),
                const_cast<uint8_t*>(characteristic->getValue().data()),
                characteristic->getValue().size());
        }

        characteristicHandles[handle] = characteristic;
        Serial.printlnf("Added characteristic handle: %d", handle);

        // add the other descriptors
        for (const std::shared_ptr<Descriptor>& descriptor : characteristic->getDescriptors()) {
            uint16_t handle;
            if (descriptor->getType().is16()) {
                handle = ble.addDescriptor(
                    descriptor->getType().data16(),
                    static_cast<uint16_t>(descriptor->getProperties()),
                    const_cast<uint8_t*>(descriptor->getValue().data()),
                    descriptor->getValue().size());
            } else {
                handle = ble.addDescriptor(
                    descriptor->getType().data128(),
                    static_cast<uint16_t>(descriptor->getProperties()),
                    const_cast<uint8_t*>(descriptor->getValue().data()),
                    descriptor->getValue().size());
            }

            descriptorHandles[handle] = descriptor;
            Serial.printlnf("Added descriptor handle: %d", handle);
        }

        // client characteristic configuration descriptor
        if (characteristic->isNotify() || characteristic->isIndicate()) {
            std::shared_ptr<MutableDescriptor> descriptor = std::make_shared<MutableDescriptor>(
                UUID(GATT_CLIENT_CHARACTERISTICS_CONFIGURATION),
                std::vector<uint8_t>{ GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NONE });
            characteristic->addDescriptor(descriptor);
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
        return value.size();
    }

    std::shared_ptr<Descriptor> descriptor = descriptorHandles[handle];
    if (descriptor) {
        const std::vector<uint8_t>& value = descriptor->getValue();
        // if buffer is null, don't copy and just return size
        if (buffer)
            memcpy(buffer, value.data(), bufferSize);
        return value.size();
    }

    Serial.println("Could not find matching characteristic or descriptor for read!");
    return 0;
}

int BLE::Manager::onWriteCallback(uint16_t handle, uint8_t* buffer, uint16_t bufferSize) {
    std::vector<uint8_t> newValue(buffer, buffer + bufferSize);

    std::shared_ptr<Characteristic> characteristic = characteristicHandles[handle];
    if (characteristic) {
        return static_cast<uint8_t>(characteristic->setValue(newValue));
    }

    std::shared_ptr<Descriptor> descriptor = descriptorHandles[handle];
    if (descriptor) {
        return static_cast<uint8_t>(descriptor->setValue(newValue));
    }

    Serial.printlnf("Could not find matching characteristic for write! Handle: %d", handle);
    return static_cast<uint8_t>(Error::UnlikelyError);
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
