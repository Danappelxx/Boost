//
//  AppDelegate.swift
//  Boost
//
//  Created by Dan Appel on 8/6/18.
//  Copyright © 2018 Dan Appel. All rights reserved.
//

import UIKit
import UserNotifications

@UIApplicationMain
class AppDelegate: UIResponder, UIApplicationDelegate, UNUserNotificationCenterDelegate {

    var window: UIWindow?

    func application(_ application: UIApplication, didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {

        // Request permission for notifications
        UNUserNotificationCenter.current()
            .requestAuthorization(options: [.alert, .sound]) { (_, _) in }

        UNUserNotificationCenter.current().delegate = self

        if let centralsObj = launchOptions?[UIApplication.LaunchOptionsKey.bluetoothCentrals], let centrals = centralsObj as? [String] {
            if centrals.contains(BluetoothManager.restorationId) {
                // singleton so just getting it will initialize it
                print("Woke up to restore manager \(BluetoothManager.restorationId)")
            }
        }

        return true
    }

    func application(_ app: UIApplication, open url: URL, options: [UIApplication.OpenURLOptionsKey : Any] = [:]) -> Bool {
        print("App opened with url: \(url)")
        if SpotifyRemoteManager.shared.authCallback(url) {
            return true
        }
        return false
    }

    func userNotificationCenter(_: UNUserNotificationCenter, didReceive response: UNNotificationResponse, withCompletionHandler completionHandler: @escaping () -> Void) {
        completionHandler()
    }

    func userNotificationCenter(_: UNUserNotificationCenter, willPresent notification: UNNotification, withCompletionHandler completionHandler: @escaping (UNNotificationPresentationOptions) -> Void) {
        completionHandler([.alert, .sound])
    }
}
