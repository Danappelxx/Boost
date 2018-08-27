from flask import Flask, request
import simplecrypt
app = Flask(__name__)

@app.route("/")
def index():
    return "Boost Spotify integration service."
