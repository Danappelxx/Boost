//
//  LEDResource.swift
//  Vortex
//
//  Created by Dan Appel on 8/6/18.
//  Copyright © 2018 Dan Appel. All rights reserved.
//

import CoreBluetooth

struct LED: GenericResourceDescription {
    typealias Value = State
    enum State: UInt8, DataConvertible {
        case off = 0
        case on = 1

        var flipped: State {
            switch self {
            case .on: return .off
            case .off: return .on
            }
        }

        var asData: Data {
            return Data(bytes: [rawValue])
        }

        init(from data: Data) throws {
            guard data.count == 1, data[0] == 0 || data[0] == 1 else {
                throw DataConvertibleError.invalidData
            }
            self.init(rawValue: data[0])!
        }
    }

    static let serviceId = CBUUID(string: "70DA7AB7-4FE2-4614-B092-2E8EC60290BB")
    static let characteristicId = CBUUID(string: "6962CDC6-DCB1-465B-8AA4-23491CAF4840")
    static let wantsNotificationsEnabled = false
}
