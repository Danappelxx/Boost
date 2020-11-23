//
//  SteeringWheelResource.swift
//  Boost
//
//  Created by Dan Appel on 8/15/18.
//  Copyright © 2018 Dan Appel. All rights reserved.
//

import CoreBluetooth

struct SteeringWheel: GenericResourceDescription {
    typealias Value = State

    enum State: UInt8, DataConvertible, CustomStringConvertible {
        case none = 0x00
        case volumeUp = 0x01
        case volumeDown = 0x02
        case rightUp = 0x03
        case rightDown = 0x04
        case leftUp = 0x05
        case leftDown = 0x06
        case set = 0x13
        case voice = 0x11
        case clear = 0x12

        var description: String {
            switch self {
            case .none: return "none"
            case .volumeUp: return "volume up"
            case .volumeDown: return "volume down"
            case .rightUp: return "right up"
            case .rightDown: return "right down"
            case .leftUp: return "left up"
            case .leftDown: return "left down"
            case .set: return "set"
            case .voice: return "voice"
            case .clear: return "clear"
            }
        }

        var asData: Data {
            return Data([rawValue])
        }

        init(from data: Data) throws {
            guard data.count == 1 else {
                throw DataConvertibleError.invalidData
            }
            self.init(rawValue: data[0])!
        }
    }

    static let serviceId = CBUUID(string: "9A7E8B1D-EA49-40D6-B575-406AD07F8816")
    static let characteristicId = CBUUID(string: "BBBEF1D2-E1E6-4189-BE0B-C00D6D3CC6BB")
    static let wantsNotificationsEnabled = true
}

