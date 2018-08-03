#pragma once

#define PLATFORM_ID 88
#include "application.h"
#include <map>
#include <memory>
#include <vector>
#include <string>

namespace BLE {
    struct UUID {
        // 16 or 128 bit uuid type
        UUID(uint16_t data);
        UUID(const uint8_t (&data)[16]);
        UUID(const std::string string);

        bool is16() const;
        bool is128() const;

        uint16_t data16() const;
        const uint8_t* data128() const;

        // len 16 = 128 bits
        // len 2 = 16 bits
        // cheating here but btstack doesn't mark anything as const ugh
        mutable std::vector<uint8_t> data;
    };

    enum class Error: uint8_t {
        OK = 0,
        InvalidHandle = ATT_ERROR_INVALID_HANDLE,
        ReadNotPermitted = ATT_ERROR_READ_NOT_PERMITTED,
        WriteNotPermitted = ATT_ERROR_WRITE_NOT_PERMITTED,
        InvalidPDU = ATT_ERROR_INVALID_PDU,
        InsufficientAuthentication = ATT_ERROR_INSUFFICIENT_AUTHENTICATION,
        RequestNotSupported = ATT_ERROR_REQUEST_NOT_SUPPORTED,
        InvalidOffset = ATT_ERROR_INVALID_OFFSET,
        InsufficientAuthorization = ATT_ERROR_INSUFFICIENT_AUTHORIZATION,
        PrepareQueueFull = ATT_ERROR_PREPARE_QUEUE_FULL,
        AttributeNotFound = ATT_ERROR_ATTRIBUTE_NOT_FOUND,
        AttributeNotLong = ATT_ERROR_ATTRIBUTE_NOT_LONG,
        InsufficientEncryptionKeySize = ATT_ERROR_INSUFFICIENT_ENCRYPTION_KEY_SIZE,
        InvalidAttributeValueLength = ATT_ERROR_INVALID_ATTRIBUTE_VALUE_LENGTH,
        UnlikelyError = ATT_ERROR_UNLIKELY_ERROR,
        InsufficientEncryption = ATT_ERROR_INSUFFICIENT_ENCRYPTION,
        UnsupportedGroupType = ATT_ERROR_UNSUPPORTED_GROUP_TYPE,
        InsufficientResources = ATT_ERROR_INSUFFICIENT_RESOURCES
    };

    enum class Properties: uint16_t {
        None = 0,
        Broadcast = ATT_PROPERTY_BROADCAST,
        Read = ATT_PROPERTY_READ,
        WriteWithoutResponse = ATT_PROPERTY_WRITE_WITHOUT_RESPONSE,
        Write = ATT_PROPERTY_WRITE,
        Notify = ATT_PROPERTY_NOTIFY,
        Indicate = ATT_PROPERTY_INDICATE,
        AuthenticatedSignedWrites = ATT_PROPERTY_AUTHENTICATED_SIGNED_WRITE,
        ExtendedProperties = ATT_PROPERTY_EXTENDED_PROPERTIES,
        Dynamic = ATT_PROPERTY_DYNAMIC
    };
    inline Properties operator | (Properties lhs, Properties rhs) {
        return static_cast<Properties>(static_cast<uint16_t>(lhs) | static_cast<uint16_t>(rhs));
    }
    inline Properties& operator |= (Properties& lhs, Properties rhs) {
        lhs = lhs | rhs;
        return lhs;
    }
    inline Properties operator & (Properties lhs, Properties rhs) {
        return static_cast<Properties>(static_cast<uint16_t>(lhs) & static_cast<uint16_t>(rhs));
    }

    class Characteristic;
    class Descriptor {
        friend class Manager;
        friend class Characteristic;

    public:
        Descriptor(const UUID& type, Properties properties): type(type), properties(properties) {}
        virtual const std::vector<uint8_t>& getValue() = 0;
        virtual Error setValue(const std::vector<uint8_t>& newValue) = 0;

        const UUID& getType() const { return type; }
        const Properties& getProperties() const { return properties; }

    protected:
        UUID type;
        Properties properties;
        std::weak_ptr<Characteristic> characteristic;
    };

    // Read-only descriptor with a constant value
    class StaticDescriptor: public Descriptor {
    public:
        StaticDescriptor(const UUID& type, const std::vector<uint8_t>& value): Descriptor(type, Properties::None), value(value) {
            this->properties |= Properties::Read;
        }
        const std::vector<uint8_t>& getValue() override { return value; }

    protected:
        std::vector<uint8_t> value;
    };

    // Read-write descriptor with a mutable value
    class MutableDescriptor: public StaticDescriptor {
    public:
        MutableDescriptor(const UUID& type, const std::vector<uint8_t>& value): StaticDescriptor(type, value) {
            this->properties |= Properties::Read | Properties::Write | Properties::Dynamic;
        }
        Error setValue(const std::vector<uint8_t>& newValue) override {
            value = newValue;
            return Error::OK;
        }
    };

    class Characteristic: public std::enable_shared_from_this<Characteristic> {
        friend class Manager;

    public:
        Characteristic(
            const UUID& type,
            Properties properties): type(type), properties(properties) {}

        void addDescriptor(std::shared_ptr<Descriptor> descriptor);

        virtual const std::vector<uint8_t>& getValue() const = 0;
        virtual Error setValue(const std::vector<uint8_t>& newValue) = 0;

        const UUID& getType() const { return type; }
        const Properties& getProperties() const { return properties; }
        const std::vector<std::shared_ptr<Descriptor>>& getDescriptors() const { return descriptors; }

        bool isDynamic() const {
            return static_cast<uint16_t>(getProperties() & Properties::Dynamic);
        }
        bool isIndicate() const {
            return static_cast<uint16_t>(getProperties() & Properties::Indicate);
        }
        bool isNotify() const {
            return static_cast<uint16_t>(getProperties() & Properties::Notify);
        }

    protected:
        UUID type;
        Properties properties;
        std::vector<std::shared_ptr<Descriptor>> descriptors;
    };

    // Read-only characteristic with a constant value
    class StaticCharacteristic: public Characteristic {
    public:
        StaticCharacteristic(const UUID& type, const std::vector<uint8_t>& value): Characteristic(type, Properties::None), value(value) {
            this->properties |= Properties::Read;
        }

        const std::vector<uint8_t>& getValue() const override { return value; }

        Error setValue(const std::vector<uint8_t>& newValue) override {
            // SHOULD NEVER HAPPEN!
            Serial.println("onWrite callback called for static characteristic!!");
            return Error::UnlikelyError;
        }

    protected:
        std::vector<uint8_t> value;
    };

    class Service {
    public:
        Service(UUID type);
        void addCharacteristic(std::shared_ptr<Characteristic> characteristic);

        const UUID& getType() const { return type; }
        const std::vector<std::shared_ptr<Characteristic>>& getCharacteristics() const { return characteristics; }

    private:
        UUID type;
        std::vector<std::shared_ptr<Characteristic>> characteristics;
        //TODO: do we need secondary services?
        //std::vector<Service> includedServices;
    };

    class Manager {
    public:
        Manager();
        ~Manager();

        void addService(Service service);

        void setAdvertisingParameters(advParams_t* advertisingParameters);
        //TODO: make wrapper around advertisement data
        void setAdvertisementData(std::vector<uint8_t>& advertisementData);
        //TODO: make wrapper around scan response data
        void setScanResponseData(std::vector<uint8_t>& scanResponseData);

        void startAdvertising();
        void stopAdvertising();

    private:
        uint16_t onReadCallback(uint16_t handle, uint8_t* buffer, uint16_t bufferSize);
        int onWriteCallback(uint16_t handle, uint8_t* buffer, uint16_t bufferSize);
        void onConnectedCallback(BLEStatus_t status, uint16_t handle);
        void onDisconnectedCallback(uint16_t handle);

        std::vector<Service> services;
        std::map<uint16_t, std::shared_ptr<Characteristic>> characteristicHandles;
        std::map<uint16_t, std::shared_ptr<Descriptor>> descriptorHandles;
    };
}
