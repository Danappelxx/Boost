#include "Bluetooth.h"
#include <string>
#include <vector>

using namespace BLE;

const uint16_t peripheralAppearance = BLE_APPEARANCE_UNKNOWN;

// BLE peripheral preferred connection parameters:
// - Minimum connection interval = MIN_CONN_INTERVAL * 1.25 ms, where MIN_CONN_INTERVAL ranges from 0x0006 to 0x0C80
// - Maximum connection interval = MAX_CONN_INTERVAL * 1.25 ms,  where MAX_CONN_INTERVAL ranges from 0x0006 to 0x0C80
// - The SLAVE_LATENCY ranges from 0x0000 to 0x03E8
// - Connection supervision timeout = CONN_SUPERVISION_TIMEOUT * 10 ms, where CONN_SUPERVISION_TIMEOUT ranges from 0x000A to 0x0C80
const uint16_t minConnectionInterval = 0x0028; // 50 ms
const uint16_t maxConnectionInterval = 0x0190; // 500 ms
const uint16_t slaveLatency = 0x0000; // no slave latency
const uint16_t connectionSupervisionTimeout = 0x03E8; // 10 s

// BLE peripheral advertising parameters:
// - advertising_interval_min: [0x0020, 0x4000], default: 0x0800, unit: 0.625 msec
// - advertising_interval_max: [0x0020, 0x4000], default: 0x0800, unit: 0.625 msec
// - advertising_type:
//       BLE_GAP_ADV_TYPE_ADV_IND
//       BLE_GAP_ADV_TYPE_ADV_DIRECT_IND
//       BLE_GAP_ADV_TYPE_ADV_SCAN_IND
//       BLE_GAP_ADV_TYPE_ADV_NONCONN_IND
// - own_address_type:
//       BLE_GAP_ADDR_TYPE_PUBLIC
//       BLE_GAP_ADDR_TYPE_RANDOM
// - advertising_channel_map:
//       BLE_GAP_ADV_CHANNEL_MAP_37
//       BLE_GAP_ADV_CHANNEL_MAP_38
//       BLE_GAP_ADV_CHANNEL_MAP_39
//       BLE_GAP_ADV_CHANNEL_MAP_ALL
// - filter policies:
//       BLE_GAP_ADV_FP_ANY
//       BLE_GAP_ADV_FP_FILTER_SCANREQ
//       BLE_GAP_ADV_FP_FILTER_CONNREQ
//       BLE_GAP_ADV_FP_FILTER_BOTH
const uint16_t minAdvertisingInterval = 0x0030; // 30 ms
const uint16_t maxAdvertisingInterval = 0x0030; // 30 ms
const uint8_t advertisingType = BLE_GAP_ADV_TYPE_ADV_IND; // fully open
const uint8_t addressType = BLE_GAP_ADDR_TYPE_PUBLIC; // 3 byte company id, 3 byte device id
const uint8_t address[BD_ADDR_LEN] = { 0x13, 0x33, 0x22, 0x00, 0x70, 0x07 }; // one 1, three 3, two 2 - double double o seven
const uint8_t channelMap = BLE_GAP_ADV_CHANNEL_MAP_ALL; // any channel
const uint8_t filterPolicy = BLE_GAP_ADV_FP_ANY; // no privacy filters
static advParams_t advertisingParameters = {
    .adv_int_min   = minAdvertisingInterval,
    .adv_int_max   = maxAdvertisingInterval,
    .adv_type      = advertisingType,
    .dir_addr_type = addressType,
    .dir_addr      = { 0x13, 0x33, 0x22, 0x00, 0x70, 0x07 },
    .channel_map   = channelMap,
    .filter_policy = filterPolicy
};
//TODO: refactor for this to not be static (should include current device information)
std::vector<uint8_t> makeAdvertisementData() {
    std::vector<uint8_t> vec = {
        0x02, // len
        BLE_GAP_AD_TYPE_FLAGS, // advertising type flag
        BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE, // never stops advertising, low energy only
        0x11, // len
        0x15, // "List of 128-bit Service Solicitation UUIDs"
        //BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_MORE_AVAILABLE, // just one service, no room for more
        // TODO: not hardcode this
        // 0x16, 0x88, 0x7F, 0xD0, 0x6A, 0x40, 0x75, 0xB5, 0xD6, 0x40, 0x49, 0xEA, 0x1D, 0x8B, 0x7E, 0x9A // CAN service uuid
    };
    std::vector<uint8_t> vec2 = UUID("89D3502B-0F36-433A-8EF4-C502AD55F8DC").data;
    // doesn't render properly without this...
    for (int i = vec2.size() - 1; i >= 0; i--) {
        vec.push_back(vec2[i]);
    }
    return vec;
}
static std::vector<uint8_t> advertisementData = makeAdvertisementData();
static std::vector<uint8_t> scanResponseData = {
    0x06,
    BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME,
    'B', 'o', 'o', 's', 't'
};

BLE::GapService::GapService(const std::string deviceName): Service(UUID(BLE_UUID_GAP)) {
    addCharacteristic(std::make_shared<StaticCharacteristic>(
        UUID(BLE_UUID_GAP_CHARACTERISTIC_DEVICE_NAME),
        std::vector<uint8_t>(deviceName.begin(), deviceName.end())));
    addCharacteristic(std::make_shared<StaticCharacteristic>(
        UUID(BLE_UUID_GAP_CHARACTERISTIC_APPEARANCE),
        std::vector<uint8_t>{ LOW_BYTE(peripheralAppearance), HIGH_BYTE(peripheralAppearance) }));
    addCharacteristic(std::make_shared<StaticCharacteristic>(
        UUID(BLE_UUID_GAP_CHARACTERISTIC_PPCP),
        std::vector<uint8_t>{
            LOW_BYTE(minConnectionInterval), HIGH_BYTE(minConnectionInterval),
            LOW_BYTE(maxConnectionInterval), HIGH_BYTE(maxConnectionInterval),
            LOW_BYTE(slaveLatency), HIGH_BYTE(slaveLatency),
            LOW_BYTE(connectionSupervisionTimeout), HIGH_BYTE(connectionSupervisionTimeout) }));
}

BLE::GattService::GattService(): Service(UUID(BLE_UUID_GATT)) {
    this->serviceChangedCharacteristic = std::make_shared<IndicateCharacteristic>(
        UUID(BLE_UUID_GATT_CHARACTERISTIC_SERVICE_CHANGED),
        std::vector<uint8_t>{ 0x00, 0x00, 0xFF, 0xFF }); // range from uint16(0x0000) to uint16(0xFFFF)
    addCharacteristic(this->serviceChangedCharacteristic);
}

std::unique_ptr<BLE::Manager> BLE::bluetooth() {
    std::unique_ptr<Manager> manager(new Manager);

    manager->setAdvertisingParameters(&advertisingParameters);
    manager->setAdvertisementData(advertisementData);
    manager->setScanResponseData(scanResponseData);

    manager->addService(std::make_shared<GapService>("Boost"));

    std::shared_ptr<GattService> gattService = std::make_shared<GattService>();
    manager->addService(gattService);

    manager->serviceChangedCharacteristic = gattService->serviceChangedCharacteristic;

    return manager;
}
