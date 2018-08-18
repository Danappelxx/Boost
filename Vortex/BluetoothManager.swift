//
//  Bluetooth.swift
//  Vortex
//
//  Created by Dan Appel on 8/6/18.
//  Copyright © 2018 Dan Appel. All rights reserved.
//

import CoreBluetooth

struct Settings {
    static let deviceName = "Vortex"
}

class ResourceRegistry {
    var availableResources: [DataResource] = []
    var resources: [CBCharacteristic:DataResource] = [:]

    func register(resource: DataResource) {
        availableResources.append(resource)
    }

    func discovered(characteristic: CBCharacteristic) {
        for resource in availableResources {
            if resource.characteristicId == characteristic.uuid {
                resource.characteristic = characteristic
                resources[characteristic] = resource
            }
        }
    }

    func disconnect(characteristic: CBCharacteristic) {
        if let resource = resources[characteristic] {
            availableResources.append(resource)
            resources[characteristic] = nil
        }
    }

    func receivedUpdate(from characteristic: CBCharacteristic) {
        guard let data = characteristic.value else {
            return print("Characteristic has no value!")
        }
        guard let resource = resources[characteristic] else {
            return print("No regirested resource for \(characteristic)!")
        }
        resource.received(data: data)
    }
}

class BluetoothManager: NSObject {

    let resourceRegistry = ResourceRegistry()

    let centralManager: CBCentralManager
    var peripheral: CBPeripheral?

    var wantsScan = false

    override init() {
        self.centralManager = CBCentralManager(delegate: nil, queue: DispatchQueue.main)
        super.init()
        self.centralManager.delegate = self
    }

    func beginScan() {
        if centralManager.state != .poweredOn {
            wantsScan = true
        } else {
            wantsScan = false
            // TODO: get service ids from registry
            print("beginning scan for peripherals")
            self.centralManager.scanForPeripherals(withServices: nil, options: nil)
        }
    }

    func stopScan() {
        wantsScan = false
        self.centralManager.stopScan()
    }

    func register(resource: DataResource) {
        resourceRegistry.register(resource: resource)
    }
}

extension BluetoothManager: CBCentralManagerDelegate {
    // MARK: Central discovery
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        print("Manager updated state", central, central.state.rawValue)

        switch central.state {
        case .poweredOff: print("powered off")
        case .poweredOn: print("powered on")
        case .resetting: print("resetting")
        case .unauthorized: print("unauthorized")
        case .unknown: print("unknown")
        case .unsupported: print("unsupported")
        }

        if central.state == .poweredOn, self.wantsScan {
            beginScan()
        }
    }

    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String:Any], rssi RSSI: NSNumber) {
        print("Discovered peripheral", peripheral, advertisementData, RSSI)
        print("Device name: \(peripheral.name ?? "[no name]")")

        if peripheral.name == Settings.deviceName || advertisementData[CBAdvertisementDataLocalNameKey] as? String == Settings.deviceName {
            self.peripheral = peripheral
            peripheral.delegate = self
            central.connect(peripheral, options: nil)
            self.stopScan()
        }
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        print("Connected to peripheral", peripheral)
        peripheral.discoverServices(nil)
    }

    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
        print("Failed to connect to peripheral", peripheral, error ?? "")

        self.peripheral = nil
        self.beginScan()
    }

    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        print("Disconnected from peripheral", peripheral, error ?? "")

        peripheral.services?
            .flatMap { $0.characteristics ?? [] }
            .forEach { resourceRegistry.disconnect(characteristic: $0) }
        self.peripheral = nil
        self.beginScan()
    }
}

extension BluetoothManager: CBPeripheralDelegate {
    // MARK: Peripheral discovery
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        print("Peripheral discovered services", peripheral, error ?? "")

        for service in peripheral.services ?? [] {
            peripheral.discoverCharacteristics(nil, for: service)
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        print("Peripheral discovered characteristics", peripheral, service, error ?? "")

        for characteristic in service.characteristics ?? [] {
            resourceRegistry.discovered(characteristic: characteristic)
            peripheral.readValue(for: characteristic)
            peripheral.discoverDescriptors(for: characteristic)
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverDescriptorsFor characteristic: CBCharacteristic, error: Error?) {
        print("Peripheral discovered descriptors", peripheral, characteristic, error ?? "")
    }

    // MARK: Peripheral updates
    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        print("Peripheral characteristic value updated", peripheral, characteristic, error ?? "")

        resourceRegistry.receivedUpdate(from: characteristic)
    }

    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor descriptor: CBDescriptor, error: Error?) {
        print("Peripheral descriptor value updated", peripheral, descriptor, error ?? "")
    }

    // MARK: Peripheral writing
    func peripheral(_ peripheral: CBPeripheral, didWriteValueFor characteristic: CBCharacteristic, error: Error?) {
        print("Peripheral characteristic value written", peripheral, characteristic, error ?? "")
    }

    func peripheral(_ peripheral: CBPeripheral, didWriteValueFor descriptor: CBDescriptor, error: Error?) {
        print("Peripheral descriptor value written", peripheral, descriptor, error ?? "")
    }

    // MARK: Peripheral notifications
    func peripheral(_ peripheral: CBPeripheral, didUpdateNotificationStateFor characteristic: CBCharacteristic, error: Error?) {
        print("Peripheral updated notification state for characteristic", peripheral, characteristic, "[\(error?.localizedDescription ?? "no error")]")
    }
}
