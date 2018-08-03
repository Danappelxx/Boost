#include "VortexBluetooth.h"
#include <string>
#include <vector>

using namespace BLE;

const std::string deviceName = "Vortex";
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
const uint8_t address[BD_ADDR_LEN] = { 0xd3, 0x33, 0x22, 0x1e, 0x30, 0xb3 }; // random.org
const uint8_t channelMap = BLE_GAP_ADV_CHANNEL_MAP_ALL; // any channel
const uint8_t filterPolicy = BLE_GAP_ADV_FP_ANY; // no privacy filters
static advParams_t advertisingParameters = {
    .adv_int_min   = minAdvertisingInterval,
    .adv_int_max   = maxAdvertisingInterval,
    .adv_type      = advertisingType,
    .dir_addr_type = addressType,
    .dir_addr      = { 0xd3, 0x33, 0x22, 0x1e, 0x30, 0xb3 },
    .channel_map   = channelMap,
    .filter_policy = filterPolicy
};
//TODO: refactor for this to not be static (should include current device information)
static std::vector<uint8_t> advertisementData = {
    0x02, // len
    BLE_GAP_AD_TYPE_FLAGS, // advertising type flag
    BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE, // never stops advertising, low energy only
};
static std::vector<uint8_t> scanResponseData = {
    0x07,
    BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME,
    'V', 'o', 'r', 't', 'e', 'x'
};

BLE::Service gapService() {
    Service gapService = Service(UUID(BLE_UUID_GAP));
    gapService.addCharacteristic(std::make_shared<StaticCharacteristic>(
        UUID(BLE_UUID_GAP_CHARACTERISTIC_DEVICE_NAME),
        std::vector<uint8_t>(deviceName.begin(), deviceName.end())));
    gapService.addCharacteristic(std::make_shared<StaticCharacteristic>(
        UUID(BLE_UUID_GAP_CHARACTERISTIC_APPEARANCE),
        std::vector<uint8_t>{ LOW_BYTE(peripheralAppearance), HIGH_BYTE(peripheralAppearance) }));
    gapService.addCharacteristic(std::make_shared<StaticCharacteristic>(
        UUID(BLE_UUID_GAP_CHARACTERISTIC_PPCP),
        std::vector<uint8_t>{
            LOW_BYTE(minConnectionInterval), HIGH_BYTE(minConnectionInterval),
            LOW_BYTE(maxConnectionInterval), HIGH_BYTE(maxConnectionInterval),
            LOW_BYTE(slaveLatency), HIGH_BYTE(slaveLatency),
            LOW_BYTE(connectionSupervisionTimeout), HIGH_BYTE(connectionSupervisionTimeout) }));
    return gapService;
}

BLE::Service gattService() {
    Service gattService = Service(UUID(BLE_UUID_GATT));
    // NOTE: we wil never change services while running, so we don't need the SERVICE_CHANGED characteristic
    return gattService;
}

std::unique_ptr<BLE::Manager> BLE::vortexBluetooth() {
    std::unique_ptr<Manager> manager(new Manager);

    manager->addService(gapService());
    manager->addService(gattService());

    manager->setAdvertisingParameters(&advertisingParameters);
    manager->setAdvertisementData(advertisementData);
    manager->setScanResponseData(scanResponseData);

    return manager;
}