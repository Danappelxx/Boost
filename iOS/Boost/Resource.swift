//
//  Resource.swift
//  Boost
//
//  Created by Dan Appel on 8/6/18.
//  Copyright © 2018 Dan Appel. All rights reserved.
//

import CoreBluetooth

enum DataConvertibleError: Error {
    case invalidData
}

protocol DataConvertible {
    var asData: Data { get }
    init(from data: Data) throws
}

enum DataResourceError: Error {
    case characteristicNotConnected
}

protocol DataResource: class {
    var serviceId: CBUUID { get }
    var characteristicId: CBUUID { get }

    var characteristic: CBCharacteristic? { get set }

    func received(data: Data)
}

extension DataResource {
    func send(value: Data) throws {
        guard let characteristic = characteristic else {
            throw DataResourceError.characteristicNotConnected
        }
        characteristic.service.peripheral.writeValue(value, for: characteristic, type: .withResponse)
    }

    func setNotificationsEnabled(_ value: Bool) throws {
        guard let characteristic = characteristic else {
            throw DataResourceError.characteristicNotConnected
        }
        characteristic.service.peripheral.setNotifyValue(value, for: characteristic)
    }
}

protocol Resource: DataResource {
    associatedtype Value: DataConvertible

    func received(value: Value)
}

extension Resource {
    func received(data: Data) {
        do {
            self.received(value: try Value(from: data))
        } catch DataConvertibleError.invalidData {
            print("Invalid data!")
        } catch {
            fatalError("Unexpected error")
        }
    }
}

protocol GenericResourceDescription {
    associatedtype Value: DataConvertible

    static var serviceId: CBUUID { get }
    static var characteristicId: CBUUID { get }
    static var wantsNotificationsEnabled: Bool { get }
}

class GenericResource<T: GenericResourceDescription>: Resource {
    public struct Callbacks {
        var receivedNewValue: ((T.Value) -> Void)?

        fileprivate init() {}
    }

    internal var characteristic: CBCharacteristic? {
        didSet {
            // should only fail if characteristic is nil, but it cannot be
            do {
                // if not true, don't bother sending the request
                if (T.wantsNotificationsEnabled) {
                    try setNotificationsEnabled(T.wantsNotificationsEnabled)
                }
            } catch DataResourceError.characteristicNotConnected {
                // set to nil, whatever, ignore
                print("resource's characteristic disconnected")
            } catch {
                print("Unexpected error while setting notification settings", error)
            }
        }
    }
    public var callbacks = Callbacks()

    public let serviceId: CBUUID = T.serviceId
    public let characteristicId: CBUUID = T.characteristicId

    public init() {}

    func received(value: T.Value) {
        self.callbacks.receivedNewValue?(value)
    }

    public func send(value: T.Value) throws {
        try self.send(value: value.asData)
    }
}
