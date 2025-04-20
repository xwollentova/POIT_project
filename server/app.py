import eventlet
eventlet.monkey_patch()

import logging, requests, signal, sys, time
from flask import Flask, render_template, jsonify, request, send_from_directory
from flask_socketio import SocketIO, emit
import os

# --- Logging nastavení ---
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s %(levelname)-8s %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S'
)

logging.info("=== Spúšťam app.py ===")

# --- Flask konfigurácia ---
app = Flask(__name__, static_folder='static', template_folder='templates')
app.config['SECRET_KEY'] = 'secret!'

socketio = SocketIO(
    app,
    cors_allowed_origins="*",
    async_mode="eventlet"
)

# --- NodeMCU IP ---
NODEMCU_IP     = '192.168.88.7'
NODEMCU_URL    = f'http://{NODEMCU_IP}/command'
NODEMCU_SENSOR = f'http://{NODEMCU_IP}/sensor'

ALLOWED = {
    'FORWARD','BACKWARD','LEFT','RIGHT','STOP',
    'MEASURE_START','MEASURE_STOP'
}

running = False
sensor_thread = None

# --- Načítanie gauge.min.js ak sa načítava ručne ---
@app.route('/static/libs/<path:filename>')
def serve_libs(filename):
    return send_from_directory(os.path.join(app.static_folder, 'libs'), filename)

# --- Index stránka ---
@app.route('/')
def index():
    return render_template('index.html')

# --- Zdravotný stav ESP ---
@app.route('/health')
def health():
    try:
        r = requests.get(NODEMCU_URL.replace('/command','/'), timeout=1)
        r.raise_for_status()
        return jsonify(status='ok', nodemcustatus=r.status_code)
    except Exception as e:
        return jsonify(status='error', error=str(e)), 503

# --- Proxy na senzor ---
@app.route('/sensor')
def sensor_proxy():
    try:
        r = requests.get(NODEMCU_SENSOR, timeout=1)
        r.raise_for_status()
        return (r.text, r.status_code, {'Content-Type':'application/json'})
    except Exception:
        return jsonify(error="Sensor fetch failed"), 502

# --- Socket.IO: DHT Namespace ---
@socketio.on('connect', namespace='/dht')
def on_connect():
    emit('dht_status', {'running': running})
    logging.info("Client connected to /dht")

@socketio.on('disconnect', namespace='/dht')
def on_disconnect():
    logging.info("Client disconnected from /dht")

@socketio.on('toggle_running', namespace='/dht')
def on_toggle():
    global running, sensor_thread
    running = not running
    state = 'Spustené' if running else 'Zastavené'
    emit('dht_status', {'running': running, 'status': state})
    logging.info(f"toggle_running → running={running}")
    if running and sensor_thread is None:
        sensor_thread = socketio.start_background_task(background_sensor_thread)

# --- Snímanie údajov zo senzora ---
def background_sensor_thread():
    while True:
        socketio.sleep(2)
        if not running:
            continue
        try:
            r = requests.get(NODEMCU_SENSOR, timeout=1)
            r.raise_for_status()
            data = r.json()
            payload = {
                'temperature': data['temperature'],
                'humidity':    data['humidity'],
                'timestamp':   round(time.time(), 1)
            }
            socketio.emit('dht_data', payload, namespace='/dht')
        except Exception as e:
            logging.error(f"Sensor read error: {e}")

# --- Ovládacie príkazy pre NodeMCU ---
@socketio.on('command')
def on_command(data):
    direction = (data.get('direction') or '').upper()
    if direction not in ALLOWED:
        logging.warning(f"Ignored invalid command: {direction}")
        return
    try:
        requests.post(NODEMCU_URL, json={'command': direction}, timeout=1)
        logging.info(f"Sent command to NodeMCU: {direction}")
    except Exception as e:
        logging.error(f"Error sending command to NodeMCU: {e}")

# --- Ukončenie cez Ctrl+C ---
def handle_sigint(sig, frame):
    logging.info("SIGINT received, exiting.")
    sys.exit(0)

# --- Spustenie aplikácie ---
if __name__ == '__main__':
    signal.signal(signal.SIGINT, handle_sigint)
    logging.info("Starting Flask‑SocketIO server on port 5000")
    socketio.run(app, host='0.0.0.0', port=5000)
