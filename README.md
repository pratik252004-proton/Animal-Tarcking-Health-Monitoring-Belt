# 🐄 Pashu-Sutra — Animal Tracking & Health Monitoring Belt

An IoT-based smart collar/belt system for livestock that tracks **location (GPS)**, monitors **vital health parameters** (body temperature, heart rate, humidity), enforces a **virtual geofence**, and identifies animals via **NFC/RFID tags** — all reported in real time over MQTT to Adafruit IO.

Built as a low-cost alternative to commercial livestock tags (like SCR by Allflex or CowAlert), using off-the-shelf ESP32 hardware.

---

## 📌 Overview

Cattle and livestock are prone to undetected illness, theft, and straying, which cause major losses for farmers. **Pashu-Sutra** addresses this with a wearable neck belt that:

- Continuously tracks the animal's **GPS location** and triggers alerts if it exits a defined **geofence**
- Monitors **heart rate (BPM)**, **body/ambient temperature**, and **humidity**
- Reports **battery voltage** so the belt can be recharged/replaced in time
- Identifies each animal uniquely using an **NFC tag** storing ID, name, breed, and owner
- Publishes all data to the cloud (Adafruit IO) via **MQTT**, over WiFi or a GSM/GPRS module

---

## ⚙️ How It Works (System Architecture)

```
   ┌─────────────────────────────┐
   │         COLLAR BELT         │
   │                              │
   │  MAX30105 (heart rate)      │
   │  SHT40 (temp + humidity)    │
   │  NEO-6M GPS / A9G module    │
   │  PN532 NFC reader           │
   │  ESP32 (main controller)    │
   │  LiPo battery + voltage     │
   │  divider                    │
   └──────────────┬───────────────┘
                  │  WiFi / GSM
                  ▼
        ┌───────────────────┐
        │   MQTT Broker      │
        │  (Adafruit IO)     │
        └─────────┬──────────┘
                  ▼
     ┌────────────────────────┐
     │ Dashboard / Mobile App │
     │ GPS · BPM · Temp ·     │
     │ Humidity · Battery ·   │
     │ Geofence · NFC ID      │
     └────────────────────────┘
```

The ESP32 continuously reads sensor data, checks the GPS coordinates against a defined geofence radius using the **Haversine formula**, and publishes structured data to individual MQTT feeds that can be visualized on an Adafruit IO dashboard or any MQTT-compatible app.

---

## 🧩 Hardware Components

| Component | Purpose | Notes |
|---|---|---|
| **ESP32 Dev Board** | Main microcontroller, WiFi/BLE | Sealed inside a weatherproof housing |
| **MAX30105 / MAX30100** | Heart rate via PPG (photoplethysmography) | Positioned at the jugular groove |
| **Adafruit SHT40** | Ambient/skin-adjacent temperature & humidity | Inner belt, skin contact |
| **DS18B20** *(alt. temp sensor)* | Waterproof probe for surface skin temperature | Probe tip facing inward |
| **NEO-6M GPS module** | Location tracking | Antenna facing skyward |
| **A9G GSM/GPS module** *(alt. connectivity)* | Cellular data + GPS where WiFi is unavailable | Used in the A9G firmware variant |
| **PN532 NFC/RFID Reader** | Animal identification (ID, name, breed, owner) | Reads/writes MIFARE Classic tags |
| **LiPo Battery (3.7V, 2000mAh)** | Power source | ~12–24h runtime |
| **Voltage divider (ADC)** | Battery level monitoring | Connected to an analog pin |

### Collar Placement

- **Temperature sensor** — inner face of the collar, probe/sensor flush against neck skin
- **Heart rate sensor** — inner belt, aligned with the jugular groove, cushioned with foam for consistent contact pressure
- **GPS antenna** — top of the housing, facing upward with a clear sky view
- **ESP32 + battery** — sealed in a weatherproof enclosure on the outer housing

Fitting follows a **2-finger-gap rule** between the belt and the neck to maintain skin contact for sensors without restricting movement or breathing.

**📄 Full hardware design notes, sensor calibration tables, and health-alert thresholds are documented in [`HARDWARE_DESIGN.md`](./HARDWARE_DESIGN.md).**

---

## 💻 Software / Firmware

The firmware is written in C++ for the Arduino/ESP32 framework. Several `.ino` sketches are included, representing different stages/variants of the system:

| File | Description |
|---|---|
| `FULL_ANIMAL_TRACKING_HEALTH_MONITORING_BELT.ino` | Full system using the **A9G GSM/GPS module** for connectivity — publishes location, geofence status, and battery data over MQTT via cellular data (no WiFi needed) |
| `FINAL_CODE.ino` | Full system using **WiFi + NEO-6M GPS** — reads heart rate (MAX30105), temperature & humidity (SHT40), GPS location, geofence status, and battery voltage; publishes everything to Adafruit IO |
| `NEW_TRACKING.ino` | Lightweight GPS + geofence + battery tracking module over WiFi (no health sensors) |
| `RFID/RFID.ino` | Writes animal identity data (ID, name, breed, owner) onto an NFC tag using the PN532 reader |
| `RFID_READ/RFID_READ.ino` | Reads the NFC tag and publishes the animal's identity to Adafruit IO over MQTT |

### Key Software Features

- **Dual connectivity support** — cellular (A9G AT-command based) or WiFi, depending on farm infrastructure
- **Geofencing** — real-time distance calculation using the Haversine formula to detect if the animal has left a defined safe radius
- **Heart-rate detection** — peak detection algorithm over IR PPG signal, averaged across a rolling sample window to reduce noise
- **MQTT publishing** — structured feeds for GPS, geofence status, heart rate, temperature, humidity, battery, and RFID identity, all independently viewable on a dashboard
- **NFC-based identity** — each animal's tag stores its ID, name, breed, and owner directly on the card, enabling offline identification even without connectivity

### Libraries Used

Bundled under `/libraries`:

- `TinyGPSPlus` — GPS NMEA parsing
- `Adafruit_MQTT_Library` — MQTT client for Adafruit IO
- `Adafruit_SHT4x_Library` — Temperature & humidity sensor
- `SparkFun_MAX3010x_Pulse_and_Proximity_Sensor_Library` — Heart rate sensor
- `Adafruit_PN532` — NFC/RFID reader
- `Adafruit_GFX_Library`, `Adafruit_SH110X`, `Adafruit_SSD1306` — Display support (OLED, optional)
- `Adafruit_BusIO`, `Adafruit_Unified_Sensor` — Sensor abstraction dependencies
- `OneWire` — For DS18B20 probe communication

---

## 🚀 Getting Started

### 1. Hardware Setup
Assemble the components as described in [`HARDWARE_DESIGN.md`](./HARDWARE_DESIGN.md) and wire them to the ESP32 as per the pin definitions at the top of each `.ino` file.

### 2. Arduino IDE Setup
- Install the **ESP32 board package** in Arduino IDE
- Copy the folders under `/libraries` into your Arduino `libraries` directory (or install the equivalent libraries via the Library Manager)

### 3. Configure Credentials
Open the relevant `.ino` file and update the placeholder values:

```cpp
#define WIFI_SSID "Your WiFi SSID"
#define WIFI_PASS "Your WiFi Password"
#define AIO_USERNAME "Your Adafruit IO Username"
#define AIO_KEY      "Your Adafruit IO Key"
```

For the A9G/cellular variant, update the APN name and MQTT broker credentials at the top of `FULL_ANIMAL_TRACKING_HEALTH_MONITORING_BELT.ino`.

Also update the geofence center coordinates to match your farm's location:

```cpp
#define GEOFENCE_LATITUDE  <your latitude>
#define GEOFENCE_LONGITUDE <your longitude>
#define GEOFENCE_RADIUS    50 // meters
```

### 4. Upload & Run
Select the appropriate ESP32 board in Arduino IDE, upload the sketch of your choice, and open the Serial Monitor at `115200` baud to observe live sensor readings and MQTT publish logs.

### 5. Dashboard
Create feeds on [Adafruit IO](https://io.adafruit.com/) matching the feed names used in the code (`gpsloc`, `geofence`, `battery`, `bpm`, `temperature`, `humidity`, `rfid`) and build a dashboard to visualize the incoming data in real time.

---

## 📊 Health Alert Thresholds

| Parameter | Normal | Warning | Alert |
|---|---|---|---|
| Temperature | 36.5–38.5°C | 38.5–39.5°C (mild fever) | >39.5°C (fever) |
| Heart Rate | 60–80 BPM | 80–100 BPM (mild stress) | >100 BPM sustained (distress) |

*(See `HARDWARE_DESIGN.md` for full calibration notes.)*

---

## 🔮 Future Enhancements

- Ear-canal temperature probe for more accurate core body temperature
- Solar charging support for extended battery life
- On-device anomaly detection (edge ML) for early illness prediction
- SMS-based alerts for areas with unreliable data connectivity
- Mobile app for farmers with multi-animal herd dashboards

---

## 📁 Repository Structure

```
Animal-Tarcking-Health-Monitoring-Belt/
├── FULL_ANIMAL_TRACKING_HEALTH_MONITORING_BELT/   # A9G (GSM/GPRS) full firmware
├── FINAL_CODE/                                    # WiFi full firmware (sensors + GPS)
├── NEW_TRACKING/                                  # WiFi lightweight GPS tracker
├── RFID/                                          # NFC tag writer
├── RFID_READ/                                     # NFC tag reader + publisher
├── HARDWARE_DESIGN.md                             # Sensor placement & design rationale
├── libraries/                                     # Bundled Arduino libraries
└── README.md
```

---

## 👤 Author

**Pratik**

---
