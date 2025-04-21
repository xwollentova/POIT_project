import eventlet
eventlet.monkey_patch()

import logging
import requests
import signal
import sys
import time
import os
import mariadb
import json
from datetime import datetime
import configparser as ConfigParser
from flask import Flask, render_template, jsonify, request, send_from_directory
from flask_socketio import SocketIO, emit

# --- Logging ---
logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)-8s %(message)s')
logging.info("=== Spúšťam app.py ===")

# --- Flask ---
app = Flask(__name__, static_folder='static', template_folder='templates')
app.config['SECRET_KEY'] = 'secret!'
socketio = SocketIO(app, cors_allowed_origins="*", async_mode="eventlet")

# --- Config.cfg načítanie ---
config = ConfigParser.ConfigParser()
config.read('config.cfg')
myport = config.getint('mysqlDB', 'port')
myhost = config.get('mysqlDB', 'host')
myuser = config.get('mysqlDB', 'user')
mypasswd = config.get('mysqlDB', 'passwd')
mydb = config.get('mysqlDB', 'db')

# --- NodeMCU ---
NODEMCU_IP     = '192.168.88.5'
NODEMCU_URL    = f'http://{NODEMCU_IP}/command'
NODEMCU_SENSOR = f'http://{NODEMCU_IP}/sensor'
ALLOWED = {'FORWARD','BACKWARD','LEFT','RIGHT','STOP','MEASURE_START','MEASURE_STOP'}

running = False
sensor_thread = None
current_session = {
    "start_time": None,
    "temperatures": [],
    "humidities": []
}

# --- Statické knižnice ---
@app.route('/static/libs/<path:filename>')
def serve_libs(filename):
    return send_from_directory(os.path.join(app.static_folder, 'libs'), filename)

# --- Web stránka ---
@app.route('/')
def index():
    return render_template('index.html')

# --- Healthcheck ---
@app.route('/health')
def health():
    try:
        r = requests.get(NODEMCU_URL.replace('/command','/'), timeout=1)
        r.raise_for_status()
        return jsonify(status='ok', nodemcustatus=r.status_code)
    except Exception as e:
        return jsonify(status='error', error=str(e)), 503
    
@app.route('/record_json/<int:record_id>')
def get_record_json(record_id):
    try:
        conn = mariadb.connect(
            host=myhost,
            port=myport,
            user=myuser,
            password=mypasswd,
            database=mydb
        )

        cur = conn.cursor()
        cur.execute("SELECT id, start_time, end_time, temperatures, humidities FROM records_json WHERE id = ?", (record_id,))
        row = cur.fetchone()
        conn.close()
        if row:
            return jsonify({
                "id": row[0],
                "start_time": row[1],
                "end_time": row[2],
                "temperatures": json.loads(row[3]),
                "humidities": json.loads(row[4])
            })
        else:
            return jsonify(error="Záznam neexistuje"), 404
    except Exception as e:
        return jsonify(error=str(e)), 500
    
@app.route('/record_file/<int:record_id>')
def get_record_file(record_id):
    try:
        filename = f"records/record_{record_id:04d}.txt"
        if not os.path.exists(filename):
            return jsonify(error="Súbor neexistuje"), 404
        with open(filename, "r", encoding="utf-8") as f:
            data = json.load(f)
        return jsonify(data)
    except Exception as e:
        return jsonify(error=str(e)), 500


# --- SocketIO: DHT ---
@socketio.on('connect', namespace='/dht')
def on_connect():
    emit('dht_status', {'running': running})
    logging.info("Client connected to /dht")

@socketio.on('disconnect', namespace='/dht')
def on_disconnect():
    logging.info("Client disconnected from /dht")

@socketio.on('toggle_running', namespace='/dht')
def toggle_running():
    global running, sensor_thread, current_session
    running = not running
    emit('dht_status', {'running': running})

    if running:
        current_session = {
            "start_time": time.time(),
            "temperatures": [],
            "humidities": []
        }
        if sensor_thread is None:
            sensor_thread = socketio.start_background_task(sensor_loop)
    else:
        try:
            conn = mariadb.connect(host=myhost, user=myuser, password=mypasswd, database=mydb)
            cur = conn.cursor()

            end_time = time.time()
            cur.execute("""
                INSERT INTO records_json (start_time, end_time, temperatures, humidities)
                VALUES (?, ?, ?, ?)
            """, (
                current_session["start_time"],
                end_time,
                json.dumps(current_session["temperatures"]),
                json.dumps(current_session["humidities"])
            ))
            conn.commit()

            record_id = cur.lastrowid
            conn.close()
            logging.info(f"Session uložená do databázy (ID {record_id}).")

            # --- Uloženie do .txt súboru ---
            os.makedirs("records", exist_ok=True)
            filename = f"records/record_{record_id:04d}.txt"
            with open(filename, 'w', encoding='utf-8') as f:
                json.dump({
                    "start_time": current_session["start_time"],
                    "end_time": end_time,
                    "temperatures": current_session["temperatures"],
                    "humidities": current_session["humidities"]
                }, f, indent=2)

            logging.info(f"Session uložená do súboru: {filename}")

        except Exception as e:
            logging.error(f"Chyba pri ukladaní session: {e}")

# --- Meranie a ukladanie ---
def sensor_loop():
    global current_session
    while True:
        socketio.sleep(2)
        if not running:
            continue
        try:
            r = requests.get(NODEMCU_SENSOR, timeout=1)
            r.raise_for_status()
            data = r.json()
            payload = {
                'timestamp': round(time.time(), 1),
                'temperature': data['temperature'],
                'humidity': data['humidity']
            }

            socketio.emit('dht_data', payload, namespace='/dht')

            # Ukladáme do session (do pamäti)
            current_session["temperatures"].append(payload["temperature"])
            current_session["humidities"].append(payload["humidity"])
        except Exception as e:
            logging.error(f"Sensor read error: {e}")

# --- Ovládanie auta ---
@socketio.on('command')
def on_command(data):
    direction = (data.get('direction') or '').upper()
    if direction not in ALLOWED:
        logging.warning(f"Neplatný príkaz: {direction}")
        return
    try:
        requests.post(NODEMCU_URL, json={'command': direction}, timeout=1)
        logging.info(f"Príkaz odoslaný: {direction}")
    except Exception as e:
        logging.error(f"Chyba pri odosielaní príkazu: {e}")

# --- Ukončenie ---
def handle_sigint(sig, frame):
    logging.info("SIGINT received, exiting.")
    sys.exit(0)

if __name__ == '__main__':
    signal.signal(signal.SIGINT, handle_sigint)
    logging.info("Spúšťam SocketIO server na porte 5000")
    socketio.run(app, host='0.0.0.0', port=5000)