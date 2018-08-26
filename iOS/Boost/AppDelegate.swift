//
//  AppDelegate.swift
//  Boost
//
//  Created by Dan Appel on 8/6/18.
//  Copyright © 2018 Dan Appel. All rights reserved.
//

import UIKit

@UIApplicationMain
class AppDelegate: UIResponder, UIApplicationDelegate {

    var window: UIWindow?

    func application(_ application: UIApplication, didFinishLaunchingWithOptions launchOptions: [UIApplicationLaunchOptionsKey: Any]?) -> Bool {
        DispatchQueue.main.async {
            SpotifyRemoteManager.shared.authenticate()
        }

        if let centralsObj = launchOptions?[UIApplicationLaunchOptionsKey.bluetoothCentrals], let centrals = centralsObj as? [String] {
            if centrals.contains(BluetoothManager.restorationId) {
                // singleton so just getting it will initialize it
                print("Woke up to restore manager \(BluetoothManager.restorationId)")
            }
        }

        return true
    }

    func application(_ app: UIApplication, open url: URL, options: [UIApplicationOpenURLOptionsKey : Any] = [:]) -> Bool {
        print("did open url \(url)")
        if SpotifyRemoteManager.shared.authCallback(url) {
            return true
        }
        return false
    }
}
