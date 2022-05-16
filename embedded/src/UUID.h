#pragma once
#include "application.h"
#include <vector>
#include <string>

struct UUID {
    // 16 or 128 bit uuid type
    UUID(uint16_t data);
    UUID(const uint8_t (&data)[16]);
    UUID(const std::string string);

    bool is16() const;
    bool is128() const;

    uint16_t data16() const;
    const uint8_t* data128() const;
    std::string toString() const;

    // len 16 = 128 bits
    // len 2 = 16 bits
    // cheating here but btstack doesn't mark anything as const ugh
    mutable std::vector<uint8_t> data;

    bool operator==(const UUID& other) {
        if (data.size() != other.data.size())
            return false;
        for (auto i = 0; i < data.size(); i++)
            if (data[i] != other.data[i])
                return false;
        return true;
    }
};
