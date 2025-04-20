import eventlet
eventlet.monkey_patch()

from flask import Flask, render_template
from flask_socketio import SocketIO
import logging
import requests
import signal
import sys

app = Flask(__name__)
app.config['SECRET_KEY'] = 'secret!'

socketio = SocketIO(
    app,
    cors_allowed_origins="*",
    async_mode="eventlet"
)

NODEMCU_URL = 'http://192.168.88.7/command'  # uprav podľa svojej IP
logging.basicConfig(level=logging.INFO)
ALLOWED = {'FORWARD', 'BACKWARD', 'LEFT', 'RIGHT', 'STOP'}

@app.route('/')
def index():
    logging.info("HTTP GET / – rendering index.html")
    return render_template('index.html')

@socketio.on('connect')
def on_connect():
    logging.info("Socket.IO client connected")

@socketio.on('command')
def on_command(data):
    direction = (data.get('direction') or '').upper()
    if direction not in ALLOWED:
        logging.warning(f"Invalid command: {direction}")
        return
    logging.info(f"Sending {direction} to NodeMCU")
    try:
        resp = requests.post(NODEMCU_URL, json={'command': direction}, timeout=2)
        resp.raise_for_status()
        logging.info(f"NodeMCU replied: {resp.text}")
    except Exception as e:
        logging.error(f"Error contacting NodeMCU: {e}")

def handle_sigint(sig, frame):
    logging.info("SIGINT received — shutting down.")
    sys.exit(0)

if __name__ == '__main__':
    # zachytá SIGINT (Ctrl+C)
    signal.signal(signal.SIGINT, handle_sigint)
    logging.info("Starting Flask‑SocketIO on http://0.0.0.0:5000")
    # beží bez reloadera a debug módu, len v jednom procese
    socketio.run(
        app,
        host='0.0.0.0',
        port=5000,
        debug=False,
        use_reloader=False
    )
