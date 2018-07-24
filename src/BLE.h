#pragma once

#define PLATFORM_ID 88
#include "application.h"
#include <vector>
#include <string>
#include <algorithm>

namespace BLE {
    class UUID {
        // 16 or 128 bit uuid type
    public:
        UUID(uint16_t data);
        UUID(const uint8_t (&data)[16]);
        UUID(const std::string string);

        uint16_t data16() const;
        uint8_t* data128() const;

        // len 16 = 128 bits
        // len 2 = 16 bits
        // cheating here but btstack doesn't mark anything as const ugh
        mutable std::vector<uint8_t> data;
    };

    enum CharacteristicProperties {
        Broadcast = ATT_PROPERTY_BROADCAST,
        Read = ATT_PROPERTY_READ,
        WriteWithoutResponse = ATT_PROPERTY_WRITE_WITHOUT_RESPONSE,
        Write = ATT_PROPERTY_WRITE,
        Notify = ATT_PROPERTY_NOTIFY,
        Indicate = ATT_PROPERTY_INDICATE,
        AuthenticatedSignedWrites = ATT_PROPERTY_AUTHENTICATED_SIGNED_WRITE,
        ExtendedProperties = ATT_PROPERTY_EXTENDED_PROPERTIES,
    };
    inline CharacteristicProperties operator|(CharacteristicProperties a, CharacteristicProperties b) {
        return static_cast<CharacteristicProperties>(static_cast<int>(a) | static_cast<int>(b));
    }

    class Characteristic {
    public:
        Characteristic(UUID type, CharacteristicProperties properties, std::vector<uint8_t> value);

        const UUID& getType() const { return type; }
        const CharacteristicProperties& getProperties() const { return properties; }
        const std::vector<uint8_t>& getValue() const { return value; }

    private:
        UUID type;
        CharacteristicProperties properties;
        std::vector<uint8_t> value;
        //TODO:
        //std::vector<Descriptor> descriptors;
    };

    class Service {
    public:
        Service(UUID type);
        void addCharacteristic(Characteristic characteristic);

        const UUID& getType() const { return type; }
        const std::vector<Characteristic>& getCharacteristics() const { return characteristics; }

    private:
        UUID type;
        std::vector<Characteristic> characteristics;
        //TODO:
        //std::vector<Service> includedServices;
    };

    class Manager {
    public:
        Manager();
        ~Manager();

        void addService(Service service);

        void startAdvertising();
        void stopAdvertising();

    private:
        std::vector<Service> services;
    };
}
