//
//  BatteryLevelResource.swift
//  Boost
//
//  Created by Dan Appel on 8/15/18.
//  Copyright © 2018 Dan Appel. All rights reserved.
//

import CoreBluetooth

struct BatteryLevel: GenericResourceDescription {
    typealias Value = Double

    static let serviceId = CBUUID(string: "9A7E8B1D-EA49-40D6-B575-406AD07F8816")
    static let characteristicId = CBUUID(string: "63F13CE9-63B0-4ED3-8EBA-27441DDFC18E")
    static let wantsNotificationsEnabled = true
}

extension Double: DataConvertible {
    init(from data: Data) throws {
        guard data.count == 2 else {
            throw DataConvertibleError.invalidData
        }
        let value = (UInt16(data[1]) << 8) | UInt16(data[0])
        self = Double(value) / 100
    }

    var asData: Data {
        let value = UInt16(self * 100)
        let lower = value & 0xff
        let higher = value >> 8
        return Data(bytes: [UInt8(lower), UInt8(higher)])
    }
}
