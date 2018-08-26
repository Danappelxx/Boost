//
//  Bluetooth.swift
//  Boost
//
//  Created by Dan Appel on 8/6/18.
//  Copyright © 2018 Dan Appel. All rights reserved.
//

import CoreBluetooth

struct Settings {
    static let deviceName = "Boost"
}

class ResourceRegistry {
    var resources: [DataResource] = []

    func register(resource: DataResource) {
        resources.append(resource)
    }

    func discovered(characteristic: CBCharacteristic) {
        for resource in resources where resource.characteristicId == characteristic.uuid {
            resource.characteristic = characteristic
        }
    }

    func disconnect(characteristic: CBCharacteristic) {
        for resource in resources where resource.characteristic == characteristic {
            resource.characteristic = nil
        }
    }

    func receivedUpdate(from characteristic: CBCharacteristic) {
        guard let data = characteristic.value else {
            return print("Characteristic has no value!")
        }
        guard let resource = resources.first(where: { $0.characteristic == characteristic }) else {
            return print("No registered resource for \(characteristic)!")
        }
        resource.received(data: data)
    }
}

class BluetoothManager: NSObject {
    static let restorationId = "app.danapp.BluetoothManager.CentralManager"

    let resourceRegistry = ResourceRegistry()

    lazy var centralManager: CBCentralManager = {
        return CBCentralManager(
            delegate: self,
            queue: DispatchQueue.main,
            options: [CBCentralManagerOptionRestoreIdentifierKey: BluetoothManager.restorationId])
    }()

    var peripheral: CBPeripheral?
    var services: [CBUUID] {
        return Array(Set(resourceRegistry.resources.map { $0.serviceId }))
    }

    // true when user calls beginScan()
    var wantsScan = false

    public struct Callbacks {
        var connected: (() -> Void)?
        var disconnected: (() -> Void)?

        fileprivate init() {}
    }
    var callbacks = Callbacks()

    func beginScan() {
        if let peripheral = self.peripheral {
            if peripheral.state == .connected || peripheral.state == .connecting {
                self.wantsScan = false
                return print("Requested scan but already connected/connecting to a peripheral")
            } else {
                self.peripheral = nil
            }
        }
        guard self.centralManager.state == .poweredOn else {
            // bluetooth not ready yet, but user already wants scan
            // beginScan() will be called again once bluetooth is ready
            self.wantsScan = true
            return print("Requested scan but bluetooth not ready")
        }
        // powered on and not connected - begin scan
        print("Beginning scan for peripherals")
        self.wantsScan = false
        self.centralManager.scanForPeripherals(withServices: self.services, options: nil)
    }

    func stopScan() {
        self.wantsScan = false
        self.centralManager.stopScan()
    }

    func register(resource: DataResource) {
        resourceRegistry.register(resource: resource)
    }
}

extension BluetoothManager: CBCentralManagerDelegate {
    // MARK: Background restoration
    func centralManager(_ central: CBCentralManager, willRestoreState dict: [String : Any]) {
        print("Manager restoring state!")
        if let peripherals = dict[CBCentralManagerRestoredStatePeripheralsKey].flatMap({ $0 as? [CBPeripheral] }) {
            guard let peripheral = peripherals.first else {
                return print("No peripherals to restore")
            }
            switch peripheral.state {
            case .connected:
                print("Restored connected peripheral")
                peripheral.delegate = self
                self.peripheral = peripheral
            case .connecting:
                print("Still connecting to restored peripheral")
                peripheral.delegate = self
                self.peripheral = peripheral
            default:
                print("Peripheral not worthy of being restored")
            }
        }
    }

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

        guard central.state == .poweredOn else {
            self.peripheral = nil
            return
        }

        if let peripheral = self.peripheral {
            switch peripheral.state {
            case .connecting:
                central.connect(peripheral, options: nil)
                return print("Finishing connecting to peripheral")
            case .connected:
                peripheral.discoverServices(self.services)
                return print("Already connected to peripheral, discovering services!")
            default:
                self.peripheral = nil
                // fallthrough to scan
            }
        }

        if self.wantsScan {
            beginScan()
            return print("Initiating scan since bluetooth is enabled")
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
        self.callbacks.connected?()

        peripheral.discoverServices(self.services)
    }

    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
        print("Failed to connect to peripheral", peripheral, error ?? "")

        self.peripheral = nil
        self.beginScan()
    }

    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        print("Disconnected from peripheral", peripheral, error ?? "")
        self.callbacks.disconnected?()

        for service in peripheral.services ?? [] {
            for characteristic in service.characteristics ?? [] {
                resourceRegistry.disconnect(characteristic: characteristic)
            }
        }

        self.peripheral = nil

        // connection requests do not expire. so, there is no need to scan a second time
        // app will be woken up when it reconnects to peripheral
        central.connect(peripheral, options: nil)
    }
}

extension BluetoothManager: CBPeripheralDelegate {
    // MARK: Peripheral discovery
    func peripheral(_ peripheral: CBPeripheral, didModifyServices invalidatedServices: [CBService]) {
        print("Peripheral updated services", peripheral, invalidatedServices)

        for service in invalidatedServices {
            for characteristic in service.characteristics ?? [] {
                resourceRegistry.disconnect(characteristic: characteristic)
            }
        }

        peripheral.discoverServices(self.services)
    }

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
