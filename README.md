# AERIQ

Home air sensor — **CO₂, temperature, humidity, pressure**. ESP32-C3 firmware, a Raspberry Pi intake API, and a local simulator.

Not a dashboard mock. The firmware talks I²C to an **SCD41**, UART to a second C3, and HTTP to a Pi that stores readings in SQLite.

Live comfort math (same family as the lab): [aeriq-comfort](https://github.com/streboreziert/aeriq-comfort) · [robertstreize.com/lab.html#co2](https://robertstreize.com/lab.html#co2)

---

## What is actually here

| Path | What it is |
|---|---|
| [`main/main.c`](main/main.c) | ESP-IDF firmware — SCD41, Wi-Fi, POST `/readings` |
| [`main.py`](main.py) / [`pi/app.py`](pi/app.py) | FastAPI + SQLite on the Pi |
| [`simulator/`](simulator) | Local C simulator when the hardware is on the bench, not on the desk |
| [`docs/`](docs) | Product notes and week plan |
| [`comfort.py`](comfort.py) | Composite indoor score from CO₂ + RH (no hardware needed) |

The old README listed `src/`, `hardware/`, `app/` as if they existed. They do not — this tree is the product.

---

## Comfort CLI

```bash
python3 comfort.py --ppm 950 --rh 38
```

Returns a band (`good` / `fair` / `poor` / `bad`) and a 0–1 score. **Not a medical device.**

---

## Pi API

```bash
pip install fastapi uvicorn pydantic
uvicorn main:app --host 0.0.0.0 --port 8000
curl -X POST http://127.0.0.1:8000/readings \
  -H 'content-type: application/json' \
  -d '{"temperature":22.1,"humidity":41,"co2":910}'
```

Put Wi-Fi credentials in NVS / `menuconfig`, not in a committed `main.c`.

---

## Hardware (as built)

- ESP32-C3 · SCD41 on I²C (SDA GPIO4, SCL GPIO5)
- UART0 between the two C3s (GPIO20/21)
- Pi as the local AP / logger

Site: [robertstreize.com](https://robertstreize.com/project.html?repo=AERIQ)

MIT · Roberts Treize
