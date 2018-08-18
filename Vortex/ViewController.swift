//
//  ViewController.swift
//  Vortex
//
//  Created by Dan Appel on 8/6/18.
//  Copyright © 2018 Dan Appel. All rights reserved.
//

import UIKit
import MediaPlayer

class ViewController: UIViewController {

    @IBOutlet weak var ledSwitch: UISwitch!
    @IBOutlet weak var steeringWheelLabel: UILabel!
    @IBOutlet weak var batteryLevelLabel: UILabel!

    let manager = BluetoothManager()
    let ledResource = GenericResource<LED>()
    let steeringWheelResource = GenericResource<SteeringWheel>()
    let batteryLevelResource = GenericResource<BatteryLevel>()

    override func viewDidLoad() {
        super.viewDidLoad()

        ledResource.callbacks.receivedNewValue = self.receivedNewValue
        steeringWheelResource.callbacks.receivedNewValue = self.receivedNewValue
        batteryLevelResource.callbacks.receivedNewValue = self.receivedNewValue

        manager.register(resource: ledResource)
        manager.register(resource: steeringWheelResource)
        manager.register(resource: batteryLevelResource)

        manager.beginScan()
    }

    func receivedNewValue(_ value: LED.Value) {
        print("new value: \(value)")
        ledSwitch.isOn = value == .on
    }

    func receivedNewValue(_ value: SteeringWheel.Value) {
        print("new steering value: \(value)")
        steeringWheelLabel.text = value.description

        switch value {
        case .rightUp:
            let player = MPMusicPlayerController.systemMusicPlayer
            player.skipToNextItem()
        case .rightDown:
            let player = MPMusicPlayerController.systemMusicPlayer
            player.skipToPreviousItem()
        default: break
        }
    }

    func receivedNewValue(_ value: BatteryLevel.Value) {
        print("new battery value: \(value)")
        batteryLevelLabel.text = String(format: "%.2f", value)
    }

    @IBAction func ledSwitchChanged(_ sender: UISwitch) {
        do {
            try ledResource.send(value: sender.isOn ? .on : .off)
        } catch DataResourceError.characteristicNotConnected {
            print("Characteristic not yet connected")
        } catch {
            fatalError("unknown error \(error)")
        }
    }
}
