#Concept Spec

## Hardware Components

- **BME280** — temperature, humidity, pressure (I²C)
- **SCD41** — CO₂ measurments
- **Pervasive Displays E2290KS0F1** — 2.90" TFT E-Paper display
- **ESP32-C3-MINI-1U (with antenna)** — low-power MCU for sensor/display control
- **Batteries** - whole setup will be powered by two AA batteries

---

## System Roles

### Raspberry Pi Zero 2 W (Base System)
**Purpose:** gateway + database + API + automation

- Receives sensor data (directly or from ESP32)
- Stores measurements in a local database
- Exposes a local network API (Wi-Fi) for the mobile app
- Handles configuration, calibration, and firmware settings

### ESP32-C3 
**Purpose:** ultra-low-power “front-end node” 

- Wakes up on schedule
- Reads BME280 + SCD41 over I²C
- Updates E-Ink display over SPI (only when needed)
- Sends a compact data packet to the Pi (UART/BLE/Wi-Fi)
- Spends most time in deep sleep

---

## Data Flow

1. Sensors measure environment (CO₂, temperature, humidity, pressure)
2. Device decides whether to refresh E-Ink (scheduled or threshold-based)
3. Measurement is logged to the database (on Pi)
4. Mobile app reads:
   - latest values
   - historical graphs
   - device status and battery
5. Mobile app or html page cnnection can push settings:
   - sampling period
   - E-Ink refresh strategy
   - calibration offsets
   - alert thresholds

---

## Core Features

### 1) Dashboard (Live)
- CO₂ ppm (primary)
- Temperature (°C), humidity (%), pressure (hPa)
- Battery level (optional if measured)
- Last update timestamp
- Device status (online/offline, busy, refresh in progress)

### 2) History & Trends
- Interactive graphs (hour/day/week/month)
- Simple statistics: min/avg/max
- Optional: “Indoor air quality score” based on CO₂ bands
- CSV export (from Pi)

### 3) Configuration
- Sampling interval (e.g., 30 s, 1 min, 5 min, 15 min)
- E-Ink refresh interval (separate from sampling)
- “Smart refresh” mode:
  - refresh only if CO₂ changes by X ppm
  - refresh only if temp changes by X °C
  - or time-based fallback every N minutes
- Sensor calibration:
  - CO₂ baseline / forced recalibration (if supported/desired)
  - temperature offset (for enclosure heat)
  - humidity offset

### 4) Alerts
- Push notification if CO₂ > threshold for N minutes
- “Ventilation reminder” suggestion when CO₂ rises fast
- Quiet hours and snooze

### 5) Device Management
- Rename device
- Firmware version + diagnostics
- Wi-Fi credentials provisioning (if needed)
- Multi-device support (future)

---

## Backend (Pi) 
### API Layer
- **Python + FastAPI**
  - Provides endpoints for latest values, history queries, config updates

### Database
- **SQLite** as default
  - stores timestamped measurements and settings

### Scheduling / Automation
- Periodic tasks:
  - data ingestion
  - health checks
  - cleanup (keep last X months)
    
### Most likely booted with Ubuntu lite.

---

## Communication Options

### App ↔ Pi
- Local Wi-Fi connection using REST API (HTTP)

### ESP32 ↔ Pi
- Local Wi-Fi connection using REST API (HTTP)

---

## Display Content (E-Ink Layout Ideas)

- Large CO₂ value centered
- Small row: temp / humidity / pressure
- Bottom: last update time + tiny battery icon
- Optional: CO₂ band indicator (green/yellow/red) using patterns (no color)
