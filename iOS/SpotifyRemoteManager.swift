//
//  SpotifyRemoteManager.swift
//  Boost
//
//  Created by Dan Appel on 8/17/18.
//  Copyright © 2018 Dan Appel. All rights reserved.
//

import SafariServices
import UserNotifications

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
        guard case let .success(value) = self else {
            return nil
        }
        return value
    }

    var error: SpotifyError? {
        guard case let .error(error) = self else {
            return nil
        }
        return error
    }
}

public class SpotifyRemoteManager {
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

    private let auth: SPTAuth = {
        let auth = SPTAuth.defaultInstance()!
        auth.clientID = "9a23191fb3f24dda88b0be476bb32a5e"
        auth.redirectURL = URL(string: "appdanappboost://")!
        auth.requestedScopes = ["user-read-playback-state", "user-modify-playback-state"]
        auth.sessionUserDefaultsKey = "spotify_session"
        auth.tokenSwapURL = URL(string: "https://boost-spotify.danapp.app/swap")
        auth.tokenRefreshURL = URL(string: "https://boost-spotify.danapp.app/refresh")
        return auth
    }()

    private var authVC: SFSafariViewController?

    // callback true if was authenticated or is now authenticated
    // callback false if failed or requires app open to authenticate
    public func authenticateIfNeeded(callback: ((_ authenticated: Bool) -> ())? = nil) {
        guard let session = auth.session else {
            beginAuthenticationFlow()
            callback?(false)
            return
        }

        if session.isValid() {
            print("Access token still valid")
            callback?(true)
        } else {
            print("Attempting to renew session")
            Notifications.send(.spotifyRenewingSession)
            auth.renewSession(session) { (error, newSession) in
                if newSession == nil {
                    Notifications.error(message: "Failed to renew session")
                    print("Failed to renew session", error ?? "[no error]")
                    if (error as NSError?)?.domain == NSURLErrorDomain {
                        print("Network error - do not attempt to authenticate in funky ways")
                    } else {
                        self.beginAuthenticationFlow()
                    }
                    callback?(false)
                } else {
                    Notifications.send(.spotifyRenewedSession)
                    print("Successfully renewed session", error ?? "")
                    callback?(true)
                }
            }
        }
    }

    private func beginAuthenticationFlow() {
        print("Authenticating user through spotify site")
        // ensure in foreground
        guard UIApplication.shared.applicationState == .active else {
            print("App in background, send notification alerting user!")
            return Notifications.send(.foregroundRequiredToAuthenticateSpotify)
        }

        self.authVC = SFSafariViewController(url: self.auth.spotifyWebAuthenticationURL())
        UIApplication.shared.delegate?.window??.rootViewController?.present(self.authVC!, animated: true, completion: nil)
    }

    public func authCallback(_ url: URL) -> Bool {
        guard auth.canHandle(url) else {
            // not spotify url
            return false
        }

        print("Spotify auth callback opened")

        auth.handleAuthCallback(withTriggeredAuthURL: url) { error, session in
            defer {
                self.authVC?.presentingViewController?.dismiss(animated: true, completion: nil)
            }

            guard let session = session else {
                Notifications.error(message: "Failed to authenticate")
                return print("failed to authenticate with error", error ?? "[no error]")
            }

            print("Successfully authenticated session", session)
        }

        return true
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
            callback()
        }
    }

    private func authorized(request: URLRequest) -> URLRequest {
        var request = request
        request.setValue("Bearer \(auth.session.accessToken!)", forHTTPHeaderField: "Authorization")
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
