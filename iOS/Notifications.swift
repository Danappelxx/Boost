//
//  Notifications.swift
//  Boost
//
//  Created by Dan Appel on 4/17/19.
//  Copyright © 2019 Dan Appel. All rights reserved.
//

import UIKit
import UserNotifications

public struct Notification {
    public static let deviceConnected = Notification(identifier: "app.danapp.boost.deviceconnected", body: "Device connected")
    public static let foregroundRequiredToAuthenticateSpotify = Notification(identifier: "app.danapp.boost.authenticate", body: "Foreground app to authenticate with Spotify")
    public static let spotifyRenewingSession = Notification(identifier: "app.danapp.boost.renewing", body: "Renewing Spotify session")
    public static let spotifyInitiatedSession = Notification(identifier: "app.danapp.boost.initiated", body: "Successfully initiated Spotify session")
    public static let spotifyRenewedSession = Notification(identifier: "app.danapp.boost.renewed", body: "Successfully renewed Spotify session")
    public static let spotifyPlayPause = Notification(identifier: "app.danapp.boost.playpause", body: "Play/Pause Spotify playback")
    public static let spotifySkip = Notification(identifier: "app.danapp.boost.skip", body: "Skip Spotify playback")
    public static let spotifyJumpToBeginning = Notification(identifier: "app.danapp.boost.jumpbeginning", body: "Jump to Beginning Spotify playback")
    public static let spotifyPrevious = Notification(identifier: "app.danapp.boost.previous", body: "Previous Spotify playback")

    var identifier: String
    var title: String?
    var body: String
    var sound: UNNotificationSound

    init(identifier: String, title: String? = nil, body: String, sound: UNNotificationSound = .default) {
        self.identifier = identifier
        self.title = title
        self.body = body
        self.sound = sound
    }
}

public class Notifications {
    public static var removeNotificationTaskId = UIBackgroundTaskIdentifier.invalid

    private static var center: UNUserNotificationCenter {
        return UNUserNotificationCenter.current()
    }

    private static let all: [Notification] = [
        .deviceConnected,
        .foregroundRequiredToAuthenticateSpotify,
        .spotifyRenewingSession,
        .spotifyInitiatedSession,
        .spotifyRenewedSession,
        .spotifyPlayPause,
        .spotifySkip,
    ]

    public class func removeAllDelivered() {
        center.removeDeliveredNotifications(withIdentifiers: Notifications.all.map { $0.identifier })
    }

    public class func send(_ notification: Notification) {
        let content = UNMutableNotificationContent()

        if let title = notification.title {
            content.title = title
        }
        content.body = notification.body
        content.sound = notification.sound

        let request = UNNotificationRequest(identifier: notification.identifier, content: content, trigger: nil)

        if removeNotificationTaskId != UIBackgroundTaskIdentifier.invalid {
            UIApplication.shared.endBackgroundTask(convertToUIBackgroundTaskIdentifier(removeNotificationTaskId.rawValue))
            removeNotificationTaskId = UIBackgroundTaskIdentifier.invalid
        }
        removeAllDelivered()

        center.add(request) { error in
            print("Added push notification request for \(notification), error: \(error?.localizedDescription ?? "[]")")

            removeNotificationTaskId = UIApplication.shared.beginBackgroundTask(expirationHandler: {
                print("[task expired] Removed \(notification)")
                center.removeDeliveredNotifications(withIdentifiers: [notification.identifier])
                UIApplication.shared.endBackgroundTask(convertToUIBackgroundTaskIdentifier(removeNotificationTaskId.rawValue))
                removeNotificationTaskId = UIBackgroundTaskIdentifier.invalid
            })

            DispatchQueue.main.asyncAfter(deadline: DispatchTime.now() + .seconds(5), execute: {
                print("Removed \(notification)")
                center.removeDeliveredNotifications(withIdentifiers: [notification.identifier])
                UIApplication.shared.endBackgroundTask(convertToUIBackgroundTaskIdentifier(removeNotificationTaskId.rawValue))
                removeNotificationTaskId = UIBackgroundTaskIdentifier.invalid
            })
        }
    }

    public class func error(message: String) {
        let content = UNMutableNotificationContent()
        content.title = "Error"
        content.body = message
        content.sound = UNNotificationSound.defaultCritical

        let request = UNNotificationRequest(identifier: "app.danapp.boost.error", content: content, trigger: nil)

        center.add(request) { error in
            print("Added push notification request, error: \(error?.localizedDescription ?? "[]")")
        }
    }
}

// Helper function inserted by Swift 4.2 migrator.
fileprivate func convertToUIBackgroundTaskIdentifier(_ input: Int) -> UIBackgroundTaskIdentifier {
	return UIBackgroundTaskIdentifier(rawValue: input)
}
