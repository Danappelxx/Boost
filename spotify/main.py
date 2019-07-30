from flask import Flask, request, jsonify
from cryptography.fernet import Fernet
from os import environ
import requests
import base64

app = Flask(__name__)

client_id = environ["SPOTIFY_CLIENT_ID"]
client_secret = environ["SPOTIFY_CLIENT_SECRET"]
encryption_secret = environ["SPOTIFY_ENCRYPTION_SECRET"]
client_callback_url = environ["SPOTIFY_CLIENT_CALLBACK_URL"]

spotify_token_endpoint = "https://accounts.spotify.com/api/token"
spotify_status_endpoint = "https://api.spotify.com/v1/me/player/currently-playing"
spotify_play_endpoint = "https://api.spotify.com/v1/me/player/play"
spotify_pause_endpoint = "https://api.spotify.com/v1/me/player/pause"

encryption = Fernet(encryption_secret)

def request_swap(auth_code):
    auth_payload = {
        "grant_type": "authorization_code",
        "redirect_uri": client_callback_url,
        "code": auth_code
    }

    response = requests.post(spotify_token_endpoint, auth=(client_id, client_secret), data=auth_payload)
    token_data = response.json()

    # if successful, encrypt the refresh token before forwarding to the client
    if response.status_code == requests.codes.ok:
        token_data["refresh_token"] = encryption.encrypt(token_data["refresh_token"].encode("utf-8")).decode("utf-8")

    return token_data, response.status_code

def request_refresh(refresh_token):
    # we gave client an encrypted refresh token, so decrypt it first
    decrypted_refresh_token = encryption.decrypt(refresh_token.encode("utf-8")).decode("utf-8")

    auth_payload = {
        "grant_type": "refresh_token",
        "refresh_token": decrypted_refresh_token
    }

    response = requests.post(spotify_token_endpoint, auth=(client_id, client_secret), data=auth_payload)
    token_data = response.json()

    return token_data, response.status_code

@app.route("/")
def index():
    return "Boost Spotify integration service."

@app.route("/swap", methods=["POST"])
def swap():
    """
    This call takes a single POST parameter, "code", which
    it combines with your client ID, secret and callback
    URL to get an OAuth token from the Spotify Auth Service,
    which it will pass back to the caller in a JSON payload.
    """
    auth_code = request.form["code"]
    token_data, status_code = request_swap(auth_code=auth_code)

    return jsonify(token_data), status_code

@app.route("/refresh", methods=["POST"])
def refresh():
    """
    Gets a new access token using the given refresh token.
    """
    refresh_token = request.form["refresh_token"]
    token_data, status_code = request_refresh(refresh_token=refresh_token)

    return jsonify(token_data), status_code

@app.route("/toggle", methods=["PUT"])
def toggle():
    """
    Asks spotify if music is currently playing, and plays/pauses when applicable.
    Response:
        - 200 OK with JSON body {is_playing:BOOL} if successfully played/paused
        - 204 NO_CONTENT if spotify is not playing anything
        - otherwise, propogates api responses and errors
    """
    auth_headers = {
        "Authorization": request.headers["Authorization"],
    }

    # https://developer.spotify.com/documentation/web-api/reference/player/get-the-users-currently-playing-track/
    status_response = requests.get(spotify_status_endpoint, headers=auth_headers)
    # no music is playing - can't play/pause that
    if status_response.status_code == requests.codes.no_content:
        return jsonify(None), requests.codes.no_content
    # ran into an api error - propogate it
    if status_response.status_code != requests.codes.ok:
        return jsonify(status_response.json()), status_response.status_code

    playpause_response = None
    is_playing = status_response.json()["is_playing"]
    if is_playing:
        # https://developer.spotify.com/documentation/web-api/reference/player/pause-a-users-playback/
        playpause_response = requests.put(spotify_pause_endpoint, headers=auth_headers)
    else:
        # https://developer.spotify.com/documentation/web-api/reference/player/start-a-users-playback/
        playpause_response = requests.put(spotify_play_endpoint, headers=auth_headers)

    if playpause_response.status_code == requests.codes.no_content:
        # indicate success by flipping `is_playing`
        return jsonify({"is_playing": not is_playing}), requests.codes.ok
    else:
        return jsonify(playpause_response.json()), playpause_response.status_code

if __name__ == "__main__":
    app.run(host='0.0.0.0', port=80)
