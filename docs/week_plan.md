
##  Roberts

| Week | Objective | Konkrēti uzdevumi (moduļi / faili) | Deliverables |
|------|----------|-------------------------------------|--------------|
| 1 | PCB bring-up & HW debug | • 3.3 V rail mērījumi (idle/load) <br>• EN / GPIO8 boot testi <br>• SPI/I2C continuity tests <br>• Power-switch MOSFET ON/OFF, leakage | ✔ ESP32 programmējams <br>✔ Stabila barošana <br>✔ Nav leakage OFF |
| 2 | Firmware skeleton | • Repo struktūra (`main/`, `hal/`, `drivers/`) <br>• `hal/gpio.c` <br>• `hal/i2c.c`, `hal/spi.c` <br>• Watchdog init | ✔ Clean build <br>✔ HAL darbojas |
| 3 | Display low-level driver | • SPI init (CS/DC/RST) <br>• `drivers/display/display_init.c` <br>• Test patterns (fill, grid) | ✔ Stabilns attēls |
| 4 | Sensor drivers | • `drivers/sensors/*.c` <br>• I2C raw read <br>• CRC / error handling <br>• Sampling timing | ✔ Sensor → RAM |
| 5 | Data pipeline I | • Data struct (`data_model.h`) <br>• Ring buffer <br>• Timestamping | ✔ Live data buffer |
| 6 | Data pipeline II | • Flash / NVS storage <br>• Power-loss recovery | ✔ Dati saglabājas |
| 7 | WiFi core | • AP + STA init <br>• `net/wifi.c` <br>• HTTP server skeleton | ✔ `/status` API |
| 8 | Network robustness | • Reconnect logic <br>• Timeout handling <br>• Heap leak check | ✔ 24 h uptime |
| 9 | System integration | • Display + sensors + WiFi <br>• Power sequencing | ✔ Full system |
| 10 | Validation | • Power profiling <br>• Sleep modes <br>• Cleanup & refactor | ✔ Release firmware |

---

## Alberts

| Week | Objective | Konkrēti uzdevumi (moduļi / faili) | Deliverables |
|------|----------|-------------------------------------|--------------|
| 1 | Sensor node enclosure | • PCB constraints <br>• Vent holes (humidity) <br>• Mounting bosses | ✔ STL v1 |
| 2 | Zero W2 OS setup | • Headless OS <br>• hostapd + dnsmasq <br>• Static IP | ✔ Hotspot OK |
| 3 | HTML UI v1 (Zero) | • `html/index.html` <br>• Basic CSS <br>• Fetch API | ✔ Static UI |
| 4 | HTML UI v2 | • Live updates <br>• Error handling <br>• Mobile layout | ✔ Responsive UI |
| 5 | ESP32 HTML UI | • Lightweight HTML <br>• Embedded assets | ✔ Direct connect |
| 6 | UI polish | • JS graphs <br>• Dark / light mode | ✔ UX ready |
| 7 | Zero W2 enclosure | • Thermal paths <br>• Cable relief | ✔ STL final |
| 8 | Zero W2 PCB | • Power input <br>• USB / GPIO breakout | ✔ Gerbers |
| 9 | System integration | • ESP ↔ Zero comms <br>• Data sync | ✔ End-to-end |
| 10 | Validation | • Long-run tests <br>• User flow tests | ✔ Demo-ready |

---

##  Repository Structure

| Path | Purpose |
|-----|--------|
| `firmware/main/` | Entry point |
| `firmware/hal/` | GPIO / I2C / SPI |
| `firmware/drivers/display/` | Display driver |
| `firmware/drivers/sensors/` | Sensor drivers |
| `firmware/net/` | WiFi + HTTP |
| `firmware/power/` | Power sequencing |
| `server/html/` | Frontend |
| `server/css/` | Styles |
| `server/js/` | JS logic |
| `server/api/` | Backend |

---

## 📝 Status Legend
- ⬜ Not started  
- 🟨 In progress  
- ✅ Completed
