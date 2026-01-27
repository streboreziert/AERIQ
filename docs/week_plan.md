## Roberts 

| Week | Development Scope | Technical Tasks (Explicit) | Definition of Done |
|------|-------------------|----------------------------|-------------------|
| 1 | Hardware bring-up only | • Verify 3.3 V rail stability (idle & load) <br>• Validate ESP32-C3 boot configuration (EN, GPIO8) <br>• Continuity tests: SPI, I2C buses <br>• Power-switch MOSFET functional test (ON/OFF, leakage) | MCU flashes reliably <br>3.3 V within spec <br>No leakage in OFF state |
| 2 | Firmware infrastructure | • Repository structure (`main/`, `hal/`, `drivers/`) <br>• GPIO abstraction layer <br>• I2C HAL initialization <br>• SPI HAL initialization <br>• Watchdog configuration | Clean compilation <br>HAL interfaces callable |
| 3 | Display driver (low-level) | • SPI configuration (CS/DC/RST) <br>• Display reset & init sequence <br>• Test pattern rendering (solid fill, grid) | Stable image output |
| 4 | Sensor drivers | • I2C sensor detection <br>• Raw data acquisition <br>• CRC / error handling <br>• Sampling timing validation | Sensor data in RAM |
| 5 | Data pipeline (volatile) | • Data structures definition <br>• Ring buffer implementation <br>• Timestamping logic | Continuous data stream |
| 6 | Data persistence | • Flash / NVS storage <br>• Data integrity checks <br>• Power-loss recovery | Data survives reset |
| 7 | Network core | • Wi-Fi AP + STA configuration <br>• HTTP server skeleton <br>• JSON response format | `/status` endpoint |
| 8 | Network robustness | • Reconnection logic <br>• Timeout handling <br>• Heap usage monitoring | 24 h stable runtime |
| 9 | System integration | • Display + sensors + Wi-Fi combined <br>• Power-up / power-down sequencing | Fully integrated node |
| 10 | Validation & release | • Power profiling <br>• Sleep mode validation <br>• Code cleanup & tagging | Release-ready firmware |

---

## Alberts 

| Week | Development Scope | Technical Tasks (Explicit) | Definition of Done |
|------|-------------------|----------------------------|-------------------|
| 1 | Sensor node enclosure | • PCB constraints <br>• Ventilation for humidity <br>• Mounting points | STL v1 |
| 2 | Zero W2 OS & networking | • Headless OS install <br>• Hostapd + dnsmasq <br>• Static IP config | Stable hotspot |
| 3 | Web UI (baseline) | • Static HTML layout <br>• CSS styling <br>• Basic fetch API | `/index.html` |
| 4 | Web UI (dynamic) | • Live updates <br>• Error reporting <br>• Mobile layout | Responsive UI |
| 5 | ESP32 local UI | • Lightweight embedded HTML <br>• Minimal JS | Direct ESP access |
| 6 | UI refinement | • Graph rendering (JS) <br>• Theme switching | Polished UX |
| 7 | Zero W2 enclosure | • Thermal considerations | STL final |
| 8 | Zero W2 custom PCB | • Power input design <br>• USB / GPIO breakout | Manufacturing files |
| 9 | System integration | • ESP ↔ Zero data flow <br>• Data synchronization | End-to-end system |
| 10 | System validation | • Long-term tests <br>• User flow validation | Demo-ready system |

---

## Repository Structure (Reference)

| Path | Responsibility |
|------|---------------|
| `firmware/main/` | Application entry point |
| `firmware/hal/` | Hardware abstraction |
| `firmware/drivers/` | Display & sensors |
| `firmware/net/` | Wi-Fi & HTTP |
| `firmware/power/` | Power sequencing |
| `server/html/` | Frontend |
| `server/css/` | Styling |
| `server/js/` | Client logic |
| `server/api/` | Backend |

---

## Status Legend
- ⬜ Not started  
- 🟨 In progress  
- ✅ Completed
