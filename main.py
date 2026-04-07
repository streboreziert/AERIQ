from fastapi import FastAPI          # Import FastAPI
from pydantic import BaseModel       # Import BaseModel for validating incoming JSON
import sqlite3                       # Import SQLite support
from pathlib import Path             # Import Path for database file paths
from datetime import datetime, UTC   # Import datetime tools for timestamping arrivals

app = FastAPI()                      # Create the FastAPI app
DB_PATH = Path("aeriq.db")           # Database file path

class ReadingIn(BaseModel):          # Define the JSON structure expected from the ESP32
    temperature: float               # Temperature must be a number
    humidity: float                  # Humidity must be a number
    co2: float                       # CO2 must be a number

def get_conn():                      # Open a database connection
    conn = sqlite3.connect(DB_PATH)  # Connect to the SQLite database file
    conn.row_factory = sqlite3.Row   # Return rows in dict-like form
    return conn                      # Give the connection back

def init_db():                       # Create the readings table if it does not exist
    with get_conn() as conn:         # Open the database safely
        conn.execute("""
            CREATE TABLE IF NOT EXISTS readings (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                temperature REAL NOT NULL,
                humidity REAL NOT NULL,
                co2 REAL NOT NULL,
                received_at TEXT NOT NULL
            )
        """)
        conn.commit()                # Save the table creation

init_db()                            # Run the table creation once at startup

@app.get("/")                        # Define GET /
def root():                          # Function for GET /
    return {"message": "AERIQ server is running"}   # Return a status message

@app.get("/health")                  # Define GET /health
def health():                        # Function for GET /health
    return {"status": "ok"}          # Return a health-check response

@app.post("/readings", status_code=201)   # Define POST /readings
def create_reading(reading: ReadingIn):   # Receive validated JSON from the ESP32
    received_at = datetime.now(UTC).isoformat(timespec="seconds")  # Timestamp arrival on the Pi

    with get_conn() as conn:         # Open the database safely
        cursor = conn.execute("""
            INSERT INTO readings (temperature, humidity, co2, received_at)
            VALUES (?, ?, ?, ?)
        """, (
            reading.temperature,
            reading.humidity,
            reading.co2,
            received_at
        ))
        conn.commit()                # Save the inserted row

    return {                         # Return confirmation JSON
        "received": True,
        "id": cursor.lastrowid,
        "received_at": received_at,
        "data": reading.model_dump()
    }

@app.get("/readings")                # Define GET /readings
def get_readings():                  # Function for reading saved rows
    with get_conn() as conn:         # Open the database safely
        rows = conn.execute("""
            SELECT id, temperature, humidity, co2, received_at
            FROM readings
            ORDER BY id DESC
        """).fetchall()

    return [dict(row) for row in rows]   # Return all rows as JSON
