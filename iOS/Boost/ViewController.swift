//
//  ViewController.swift
//  Boost
//
//  Created by Dan Appel on 8/6/18.
//  Copyright © 2018 Dan Appel. All rights reserved.
//

import UIKit
import MediaPlayer

class ViewController: UIViewController {

    @IBOutlet weak var connectionStatusLabel: UILabel!
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

        manager.callbacks.connected = self.deviceConnected
        manager.callbacks.disconnected = self.deviceDisconnected

        manager.register(resource: ledResource)
        manager.register(resource: steeringWheelResource)
        manager.register(resource: batteryLevelResource)

        manager.beginScan()
    }

    func deviceConnected() {
        connectionStatusLabel.text = "connected"
    }

    func deviceDisconnected() {
        connectionStatusLabel.text = "disconnected"
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
            SpotifyRemoteManager.shared.next()
        case .rightDown:
            SpotifyRemoteManager.shared.togglePlaying()
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

    @IBAction func playPauseButtonPressed(_ sender: UIButton) {
        SpotifyRemoteManager.shared.togglePlaying()
    }

    @IBAction func previousButtonPressed(_ sender: UIButton) {
        SpotifyRemoteManager.shared.jumpToBeginning()
    }

    @IBAction func nextButtonPressed(_ sender: UIButton) {
        SpotifyRemoteManager.shared.next()
    }
}
