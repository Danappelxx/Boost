#include "VortexManager.h"

using namespace BLE;

const std::string deviceName = "Vortex";
const uint16_t peripheralAppearance = BLE_APPEARANCE_UNKNOWN;

// BLE peripheral preferred connection parameters:
// - Minimum connection interval = MIN_CONN_INTERVAL * 1.25 ms, where MIN_CONN_INTERVAL ranges from 0x0006 to 0x0C80
// - Maximum connection interval = MAX_CONN_INTERVAL * 1.25 ms,  where MAX_CONN_INTERVAL ranges from 0x0006 to 0x0C80
// - The SLAVE_LATENCY ranges from 0x0000 to 0x03E8
// - Connection supervision timeout = CONN_SUPERVISION_TIMEOUT * 10 ms, where CONN_SUPERVISION_TIMEOUT ranges from 0x000A to 0x0C80
const uint16_t minConnectionInterval = 0x0028; // 50ms
const uint16_t maxConnectionInterval = 0x0190; // 500ms
const uint16_t slaveLatency = 0x0000; // no slave latency
const uint16_t connectionSupervisionTimeout = 0x03E8; // 10s

BLE::Service gapService() {
    Service gapService = Service(UUID(BLE_UUID_GAP));
    gapService.addCharacteristic(Characteristic(
        UUID(BLE_UUID_GAP_CHARACTERISTIC_DEVICE_NAME),
        Read|Write,
        std::vector<uint8_t>(deviceName.begin(), deviceName.end())));
    gapService.addCharacteristic(Characteristic(
        UUID(BLE_UUID_GAP_CHARACTERISTIC_APPEARANCE),
        Read,
        { LOW_BYTE(peripheralAppearance), HIGH_BYTE(peripheralAppearance) }));
    gapService.addCharacteristic(Characteristic(
        UUID(BLE_UUID_GAP_CHARACTERISTIC_PPCP),
        Read,
        {
            LOW_BYTE(minConnectionInterval), HIGH_BYTE(minConnectionInterval),
            LOW_BYTE(maxConnectionInterval), HIGH_BYTE(maxConnectionInterval),
            LOW_BYTE(slaveLatency), HIGH_BYTE(slaveLatency),
            LOW_BYTE(connectionSupervisionTimeout), HIGH_BYTE(connectionSupervisionTimeout)
        }));
    return gapService;
}

BLE::Service gattService() {
    Service gattService = Service(UUID(BLE_UUID_GATT));
    gattService.addCharacteristic(Characteristic(
        UUID(BLE_UUID_GATT_CHARACTERISTIC_SERVICE_CHANGED),
        Indicate,
        { 0x00, 0x00, 0xFF, 0xFF }));
    return gattService;
}

BLE::Manager* makeManager() {
    Manager* manager = new Manager();

    manager->addService(gapService());
    manager->addService(gattService());

    return manager;
}
