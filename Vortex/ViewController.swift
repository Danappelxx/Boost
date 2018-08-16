//
//  ViewController.swift
//  Vortex
//
//  Created by Dan Appel on 8/6/18.
//  Copyright © 2018 Dan Appel. All rights reserved.
//

import UIKit

class ViewController: UIViewController {

    @IBOutlet weak var ledSwitch: UISwitch!

    let manager = BluetoothManager()
    let ledResource = GenericResource<LED>()

    override func viewDidLoad() {
        super.viewDidLoad()

        ledResource.callbacks.receivedNewValue = self.receivedNewValue

        manager.register(resource: ledResource)
        manager.beginScan()
    }

    func receivedNewValue(_ value: LED.State) {
        print("new value: \(value)")
        ledSwitch.isOn = value == .on
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
