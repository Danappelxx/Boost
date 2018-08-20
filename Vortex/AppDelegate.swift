//
//  AppDelegate.swift
//  Vortex
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
            SpotifyRemoteManager.shared.authorize()
        }

        return true
    }

    func application(_ app: UIApplication, open url: URL, options: [UIApplicationOpenURLOptionsKey : Any] = [:]) -> Bool {
        SpotifyRemoteManager.shared.didOpenUrl(url)

        return true
    }

    func applicationWillResignActive(_ application: UIApplication) {
        SpotifyRemoteManager.shared.disconnect()
    }

    func applicationDidBecomeActive(_ application: UIApplication) {
        SpotifyRemoteManager.shared.connect()
    }

}
