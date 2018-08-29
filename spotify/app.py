from flask import Flask, request, jsonify
from cryptography.fernet import Fernet
from os import environ
import requests
import base64

app = Flask(__name__)

client_id = environ["CLIENT_ID"]
client_secret = environ["CLIENT_SECRET"]
encryption_secret = environ["ENCRYPTION_SECRET"]
client_callback_url = environ["CLIENT_CALLBACK_URL"]

spotify_token_endpoint = "https://accounts.spotify.com/api/token"
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
