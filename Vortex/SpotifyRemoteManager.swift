//
//  SpotifyRemoteManager.swift
//  Vortex
//
//  Created by Dan Appel on 8/17/18.
//  Copyright © 2018 Dan Appel. All rights reserved.
//

public class SpotifyRemoteManager: NSObject, SPTAppRemoteDelegate, SPTAppRemotePlayerStateDelegate {
    public static let shared = SpotifyRemoteManager()
    private static let accessTokenKey = "spotifyAccessToken"

    private lazy var appRemote: SPTAppRemote = {
        let connectionParams = SPTAppRemoteConnectionParams(
            clientIdentifier: "9a23191fb3f24dda88b0be476bb32a5e",
            redirectURI: "appdanappvortex://",
            name: "Vortex",
            accessToken: UserDefaults.standard.string(forKey: SpotifyRemoteManager.accessTokenKey),
            defaultImageSize: CGSize.zero,
            imageFormat: .any)
        let appRemote = SPTAppRemote(connectionParameters: connectionParams, logLevel: .debug)
        appRemote.delegate = self
        return appRemote
    }()

    private var latestState: SPTAppRemotePlayerState?

    public var playing: Bool {
        guard let latestState = self.latestState else {
            return false
        }
        return !latestState.isPaused
    }

    public func authorize() {

        if appRemote.connectionParameters.accessToken != nil {
            return print("Cached access token")
        }

        // blank string = play last song
        let previousSongUri = ""
        let isInstalled = appRemote.authorizeAndPlayURI(previousSongUri)

        if !isInstalled {
            print("Spotify not installed!")
        }
    }

    public func didOpenUrl(_ url: URL) {
        guard let parameters = appRemote.authorizationParameters(from: url) else {
            // not spotify url
            return
        }

        if let accessToken = parameters[SPTAppRemoteAccessTokenKey] {
            appRemote.connectionParameters.accessToken = accessToken
        } else if let errorDescription = parameters[SPTAppRemoteErrorDescriptionKey] {
            debugPrint(errorDescription)
        }
    }

    public func connect() {
        guard appRemote.connectionParameters.accessToken != nil else {
            return print("Attempted to connect to spotify but not authorized yet")
        }

        appRemote.connect()
    }

    public func disconnect() {
        if appRemote.isConnected {
            appRemote.disconnect()
        }
    }

    public func next() {
        appRemote.playerAPI?.skip(toNext: nil)
    }

    public func previous() {
        appRemote.playerAPI?.skip(toPrevious: nil)
    }

    public func restartSong() {
        appRemote.playerAPI?.seek(toPosition: 0, callback: nil)
    }

    public func resume() {
        appRemote.playerAPI?.resume(nil)
    }

    public func pause() {
        appRemote.playerAPI?.pause(nil)
    }

    public func appRemoteDidEstablishConnection(_ appRemote: SPTAppRemote) {
        print("Spotify connection established!")
        UserDefaults.standard.set(appRemote.connectionParameters.accessToken, forKey: SpotifyRemoteManager.accessTokenKey)
        appRemote.playerAPI?.delegate = self
    }

    public func appRemote(_ appRemote: SPTAppRemote, didDisconnectWithError error: Error?) {
        print("Spotify disconnected", error ?? "[no error]")
    }

    public func appRemote(_ appRemote: SPTAppRemote, didFailConnectionAttemptWithError error: Error?) {
        print("Spotify failed connection attempt!", error ?? "[no error]")

        // check if we cached the access token. if we did, uncache and try again
        if UserDefaults.standard.string(forKey: SpotifyRemoteManager.accessTokenKey) != nil {
            UserDefaults.standard.set(nil, forKey: SpotifyRemoteManager.accessTokenKey)
            appRemote.connectionParameters.accessToken = nil
            authorize()
        }
    }

    public func playerStateDidChange(_ playerState: SPTAppRemotePlayerState) {
        print("Player state changed!")
        self.latestState = playerState
    }
}
