//
//  SpotifyRemoteManager.swift
//  Boost
//
//  Created by Dan Appel on 8/17/18.
//  Copyright © 2018 Dan Appel. All rights reserved.
//

import SafariServices
import UserNotifications
import SpotifyiOS

enum SpotifyError: Error {
    case notAuthenticated
    case networkError
    case apiError(code: Int)
    case decodingError(DecodingError)

    static var successCodes: [Int] = [200, 204]
}

enum SpotifyResult<T> {
    case success(value: T, code: Int)
    case error(SpotifyError)

    var value: (value: T, code: Int)? {
        guard case let .success(value, code) = self else {
            return nil
        }
        return (value: value, code: code)
    }

    var error: SpotifyError? {
        guard case let .error(error) = self else {
            return nil
        }
        return error
    }
}

public class SpotifyRemoteManager: NSObject {
    public static let shared = SpotifyRemoteManager()
    private static let apiRootURL = "https://api.spotify.com/v1"
    private static let apiPlayerURL = apiRootURL + "/me/player"
    private static let apiStatusURL = URL(string: apiPlayerURL + "/currently-playing")!
    private static let apiPlayURL = URL(string: apiPlayerURL + "/play")!
    private static let apiPauseURL = URL(string: apiPlayerURL + "/pause")!
    private static let apiNextURL = URL(string: apiPlayerURL + "/next")!
    private static let apiPrevURL = URL(string: apiPlayerURL + "/previous")!
    private static let apiSeekURL = URL(string: apiPlayerURL + "/seek")!
    private static let apiToggleURL = URL(string: "https://boost-spotify.danapp.app/toggle")!

    private let scope: SPTScope = [.userReadPlaybackState, .userModifyPlaybackState]
    private let configuration: SPTConfiguration = {
        let auth = SPTConfiguration(clientID: "9a23191fb3f24dda88b0be476bb32a5e", redirectURL: URL(string: "appdanappboost://")!)
        auth.tokenSwapURL = URL(string: "https://boost-spotify.danapp.app/swap")
        auth.tokenRefreshURL = URL(string: "https://boost-spotify.danapp.app/refresh")
        return auth
    }()

    private lazy var sessionManager: SPTSessionManager = SPTSessionManager(configuration: configuration, delegate: self)

    // callback true if was authenticated or is now authenticated
    // callback false if failed or requires app open to authenticate
    public func authenticateIfNeeded(callback: ((_ authenticated: Bool) -> ())? = nil) {
        if sessionManager.session == nil {
            // attempt to restore from cache
            sessionManager.session = cachedSession()
        }

        guard let session = sessionManager.session else {
            beginAuthenticationFlow()
            callback?(false)
            return
        }

        guard !session.isExpired else {
            print("Attempting to renew session")
            Notifications.send(.spotifyRenewingSession)
            sessionManager.renewSession()
            callback?(false)
            return
        }

        print("Access token still valid")
        callback?(true)
    }

    private func beginAuthenticationFlow() {
        print("Authenticating user through spotify sdk")
        // ensure in foreground
        guard UIApplication.shared.applicationState == .active else {
            print("App in background, send notification alerting user!")
            return Notifications.send(.foregroundRequiredToAuthenticateSpotify)
        }

        sessionManager.initiateSession(with: scope, options: .clientOnly)
    }

    public func authCallback(_ application: UIApplication, open url: URL, options: [UIApplication.OpenURLOptionsKey : Any]) {
        sessionManager.application(application, open: url, options: options)
    }

    private func api(request: URLRequest, callback: @escaping (SpotifyResult<Data>) -> ()) {
        authenticateIfNeeded { authenticated in
            guard authenticated else {
                print("Spotify not yet authenticated")
                return callback(.error(.notAuthenticated))
            }

            let request = self.authorized(request: request)
            let task = URLSession.shared.dataTask(with: request) { data, response, error in
                if let error = error {
                    print("Network error! \(error)")
                    return callback(.error(.networkError))
                }
                // if there was no error then there is definitely an http response
                let response = response as! HTTPURLResponse
                if SpotifyError.successCodes.contains(response.statusCode) {
                    return callback(.success(value: data ?? Data(), code: response.statusCode))
                } else {
                    return callback(.error(.apiError(code: response.statusCode)))
                }
            }
            task.resume()
        }
    }

    private func api<T: Decodable>(request: URLRequest, callback: @escaping (SpotifyResult<T>) -> ()) {
        api(request: request) { result in
            switch result {
            case let .error(error):
                Notifications.error(message: "\(error)")
                return callback(.error(error))
            case let .success(value: data, code: code):
                let decoder = JSONDecoder()
                decoder.keyDecodingStrategy = .convertFromSnakeCase
                do {
                    let result = try decoder.decode(T.self, from: data)
                    callback(.success(value: result, code: code))
                } catch let error as DecodingError {
                    callback(.error(.decodingError(error)))
                } catch {
                    // cannot happen
                    fatalError("JSONDecoder.decode can only throw a DecodingError")
                }
            }
        }
    }

    private func api(request: URLRequest, callback: @escaping () -> ()) {
        api(request: request) { data in
            if case let .error(error) = data {
                Notifications.error(message: "\(error)")
            }
            callback()
        }
    }

    private func authorized(request: URLRequest) -> URLRequest {
        var request = request
        request.setValue("Bearer \(sessionManager.session!.accessToken)", forHTTPHeaderField: "Authorization")
        return request
    }

    public func next() {
        Notifications.send(.spotifySkip)

        var nextRequest = URLRequest(url: SpotifyRemoteManager.apiNextURL)
        nextRequest.httpMethod = "POST"

        api(request: nextRequest) {
            print("Skipped to next song")
        }
    }

    public func jumpToBeginning() {
        Notifications.send(.spotifyJumpToBeginning)

        var components = URLComponents(url: SpotifyRemoteManager.apiSeekURL, resolvingAgainstBaseURL: false)!
        components.queryItems = [URLQueryItem(name: "position_ms", value: "0")]
        let url = components.url!

        var seekRequest = URLRequest(url: url)
        seekRequest.httpMethod = "PUT"

        api(request: seekRequest) {
            print("Jumped to beginning of song")
        }
    }

    public func previous() {
        Notifications.send(.spotifyPrevious)

        var prevRequest = URLRequest(url: SpotifyRemoteManager.apiPrevURL)
        prevRequest.httpMethod = "POST"

        api(request: prevRequest) {
            print("Skipped to previous song")
        }
    }

    public func play() {
        var playRequest = URLRequest(url: SpotifyRemoteManager.apiPlayURL)
        playRequest.httpMethod = "PUT"
        self.api(request: playRequest) {
            print("Sent play request")
        }
    }

    public func pause() {
        var pauseRequest = URLRequest(url: SpotifyRemoteManager.apiPauseURL)
        pauseRequest.httpMethod = "PUT"
        api(request: pauseRequest) {
            print("Sent pause request")
        }
    }

    public func togglePlaying() {
        Notifications.send(.spotifyPlayPause)

        struct Status: Codable {
            let isPlaying: Bool
        }

        var toggleRequest = URLRequest(url: SpotifyRemoteManager.apiToggleURL)
        toggleRequest.httpMethod = "PUT"

        api(request: toggleRequest) { (result: SpotifyResult<Status?>) in
            guard let (newStatus, statusCode) = result.value else {
                return print(result.error!)
            }
            if statusCode == 200, let isPlaying = newStatus?.isPlaying {
                print("Toggled playing, new state: [playing: \(isPlaying)]")
            } else if statusCode == 204 {
                print("Failed to toggle as nothing is playing")
            } else {
                fatalError("Result should not be a success if status code is non-[200,204]")
            }
        }
    }
}

extension SpotifyRemoteManager: SPTSessionManagerDelegate {
    public func sessionManager(manager: SPTSessionManager, didInitiate session: SPTSession) {
        Notifications.send(.spotifyInitiatedSession)
        cache(session: session)
    }

    public func sessionManager(manager: SPTSessionManager, didRenew session: SPTSession) {
        Notifications.send(.spotifyRenewedSession)
        cache(session: session)
    }

    public func sessionManager(manager: SPTSessionManager, didFailWith error: Error) {
        Notifications.error(message: "\(error)")
    }
}

extension SpotifyRemoteManager {

    private static let sessionCacheKey = "app.danapp.boost.spotify.session"

    private func cache(session: SPTSession) {
        do {
            let sessionData = try NSKeyedArchiver.archivedData(withRootObject: session, requiringSecureCoding: false)
            UserDefaults.standard.set(sessionData, forKey: Self.sessionCacheKey)
        } catch {
            Notifications.error(message: "Failed to archive session")
        }
    }

    private func cachedSession() -> SPTSession? {
        guard let sessionData = UserDefaults.standard.data(forKey: Self.sessionCacheKey) else {
            return nil
        }
        guard let session = try? NSKeyedUnarchiver.unarchivedObject(ofClass: SPTSession.self, from: sessionData) else {
            return nil
        }
        return session
    }
}
