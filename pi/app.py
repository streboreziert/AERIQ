import sqlite3
import time
from flask import Flask, request, jsonify, render_template

app = Flask(__name__)
DB = "/home/pi/aeriq/aeriq.db"


def get_db():
    conn = sqlite3.connect(DB)
    conn.row_factory = sqlite3.Row
    return conn


def init_db():
    with get_db() as db:
        db.execute("""
            CREATE TABLE IF NOT EXISTS readings (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                ts REAL NOT NULL,
                temperature REAL,
                humidity REAL,
                co2 INTEGER,
                pressure REAL,
                bme_temperature REAL,
                bme_humidity REAL
            )
        """)


@app.route("/readings", methods=["POST"])
def post_reading():
    data = request.get_json(force=True)
    now = time.time()
    with get_db() as db:
        db.execute(
            "INSERT INTO readings (ts, temperature, humidity, co2, pressure, bme_temperature, bme_humidity) VALUES (?,?,?,?,?,?,?)",
            (
                now,
                data.get("temperature"),
                data.get("humidity"),
                data.get("co2"),
                data.get("pressure"),
                data.get("bme_temperature"),
                data.get("bme_humidity"),
            ),
        )
        # Prune entries older than 24 h
        db.execute("DELETE FROM readings WHERE ts < ?", (now - 86400,))
    return jsonify({"status": "ok"})


@app.route("/readings", methods=["GET"])
def get_latest():
    with get_db() as db:
        row = db.execute(
            "SELECT * FROM readings ORDER BY id DESC LIMIT 1"
        ).fetchone()
    if row:
        return jsonify(dict(row))
    return jsonify({})


@app.route("/api/history")
def history():
    period = request.args.get("period", "hour")
    now = time.time()
    since = now - (3600 if period == "hour" else 86400)
    with get_db() as db:
        rows = db.execute(
            "SELECT * FROM readings WHERE ts > ? ORDER BY ts DESC", (since,)
        ).fetchall()
    return jsonify([dict(r) for r in rows])


@app.route("/")
def dashboard():
    return render_template("index.html")


if __name__ == "__main__":
    init_db()
    app.run(host="0.0.0.0", port=8000)
