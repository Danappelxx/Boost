//
//  SpotifyRemoteManager.swift
//  Vortex
//
//  Created by Dan Appel on 8/17/18.
//  Copyright © 2018 Dan Appel. All rights reserved.
//

import SafariServices

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

    private let auth: SPTAuth = {
        let auth = SPTAuth.defaultInstance()!
        auth.clientID = "9a23191fb3f24dda88b0be476bb32a5e"
        auth.redirectURL = URL(string: "appdanappvortex://")!
        auth.requestedScopes = ["user-read-playback-state", "user-modify-playback-state"]
        auth.sessionUserDefaultsKey = "spotify_session"
        auth.tokenSwapURL = URL(string: "https://vortex-spotify.danapp.app/swap")
        auth.tokenRefreshURL = URL(string: "https://vortex-spotify.danapp.app/refresh")
        return auth
    }()

    private var authVC: SFSafariViewController?

    public func authenticate() {
        guard let session = auth.session else {
            return beginAuthenticationFlow()
        }

        if session.isValid() {
            print("Access token still valid")
        } else {
            print("Attempting to renew session")
            auth.renewSession(session) { (error, newSession) in
                if newSession == nil {
                    print("Failed to renew session", error ?? "[no error]")
                    self.beginAuthenticationFlow()
                } else {
                    print("Successfully renewed session", error ?? "")
                }
            }
        }
    }

    private func beginAuthenticationFlow() {
        print("Authenticating user through spotify site")
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
            guard let session = session else {
                return print("failed to authenticate with error", error ?? "[no error]")
            }

            print("Successfully authenticated session", session)
            self.authVC?.presentingViewController?.dismiss(animated: true, completion: nil)
        }

        return true
    }

    @discardableResult
    private func api(request: URLRequest, callback: @escaping (Data?) -> ()) -> URLSessionDataTask {
        let request = authorized(request: request)
        let task = URLSession.shared.dataTask(with: request) { data, response, error in
            if let error = error {
                return print("Spotify api error!, error: \(error), response: \(response?.debugDescription ?? "[no response]")")
            }
            let response = response! as! HTTPURLResponse
            print(response)
            callback(data)
        }
        task.resume()
        return task
    }

    @discardableResult
    private func api<T: Decodable>(request: URLRequest, callback: @escaping (T) -> ()) -> URLSessionDataTask {
        return api(request: request) { data in
            let decoder = JSONDecoder()
            decoder.keyDecodingStrategy = .convertFromSnakeCase
            guard let result = try? decoder.decode(T.self, from: data!) else {
                return print("Failed to decode response", String(data: data!, encoding: .utf8) ?? "[failed decoding data]")
            }
            callback(result)
        }
    }

    @discardableResult
    private func api(request: URLRequest, callback: @escaping () -> ()) -> URLSessionDataTask {
        return api(request: request) { data in
            callback()
        }
    }

    private func authorized(request: URLRequest) -> URLRequest {
        var request = request
        request.setValue("Bearer \(auth.session.accessToken!)", forHTTPHeaderField: "Authorization")
        return request
    }

    public func next() {
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
        struct Status: Codable {
            let isPlaying: Bool
        }

        var statusRequest = URLRequest(url: SpotifyRemoteManager.apiStatusURL)
        statusRequest.httpMethod = "GET"

        api(request: statusRequest) { (status: Status) in
            switch status.isPlaying {
            case true:
                print("player status: playing")
                // playing, so pause it
                self.pause()

            case false:
                print("player status: paused")
                // paused, so play it
                self.play()
            }
        }
    }
}
