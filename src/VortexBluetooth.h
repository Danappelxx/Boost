#pragma once

#include "BLE.h"
#include <memory>

namespace BLE {
    std::unique_ptr<BLE::Manager> vortexBluetooth();
}
