#pragma once

#include "BLE.h"
#include <memory>
#include <string>

namespace BLE {

    class GapService: public Service {
    public:
        GapService(const std::string deviceName);
    };

    class GattService: public Service {
    public:
        GattService();
        std::shared_ptr<IndicateCharacteristic> serviceChangedCharacteristic;
    };

    std::unique_ptr<BLE::Manager> bluetooth();
}
