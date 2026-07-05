# 🛠️ Pashu-Sutra — Technology Stack

A detailed breakdown of every technology used in the Pashu-Sutra Smart Livestock Management System, along with the reasoning behind each choice.

---

## 📱 Mobile Application

### React Native (via Expo SDK 54)
| | |
|---|---|
| **What** | Open-source framework for building native mobile apps using JavaScript/React |
| **Why** | Write once, run on Android + iOS + Web from a single codebase. India's livestock farmers use a wide variety of Android devices; React Native ensures compatibility without separate native codebases. Expo simplifies build, OTA updates, and device API access without needing Android Studio or Xcode for development. |
| **Used for** | Entire mobile application UI and logic |

---

### Expo Go / Expo CLI
| | |
|---|---|
| **What** | Toolchain and runtime that wraps React Native with pre-built native modules |
| **Why** | Eliminates the need to manage native build environments (Gradle/Xcode). Team members can test the live app by scanning a QR code on any phone — dramatically reduces onboarding time. Expo also provides managed access to device APIs (camera, notifications, print, sharing) without writing native code. |
| **Used for** | Development server, QR-based testing, PDF generation (`expo-print`), file sharing (`expo-sharing`) |

---

### React Navigation v6
| | |
|---|---|
| **What** | Routing and navigation library for React Native |
| **Why** | Industry standard for React Native navigation. The `createBottomTabNavigator` maps perfectly to the app's multi-feature structure (Dashboard, Milk, Pregnancy, etc.). Full support for header customization (e.g., gear icon for Settings), deep linking, and screen transitions. |
| **Used for** | Bottom tab bar (7 tabs) + Settings accessible via header icon |

---

### React Native Paper (MD3)
| | |
|---|---|
| **What** | Material Design 3 component library for React Native |
| **Why** | Provides production-quality, accessible UI components (Cards, Buttons, FABs, Chips, Snackbars) that follow Google's Material You design language — familiar and intuitive for Android users. Eliminates building basic UI from scratch. |
| **Used for** | Buttons, Cards, Modal bottom-sheets, Snackbars, FABs across all screens |

---

### react-native-maps
| | |
|---|---|
| **What** | Native map component (Google Maps on Android, Apple Maps on iOS) |
| **Why** | GPS tracking is a core feature. Native maps render GPU-accelerated tiles smoothly even on low-end Android devices. Shows real-time animal location, custom geofence circle overlay, and multi-animal markers. |
| **Used for** | Dashboard live GPS map, geofence visualization |

---

### react-native-chart-kit
| | |
|---|---|
| **What** | SVG-based charting library for React Native |
| **Why** | Lightweight, no native dependencies (pure JS/SVG), works seamlessly with Expo without ejecting. Renders smooth line charts for sensor history and milk production trends. |
| **Used for** | Temperature/BPM/Battery history charts (Analytics), 14-day milk yield chart (Milk Log) |

---

### @expo/vector-icons (MaterialCommunityIcons)
| | |
|---|---|
| **What** | Icon library bundled with Expo, providing 7,000+ Material Design icons |
| **Why** | Zero configuration, no native linking required. MaterialCommunityIcons has purpose-built icons for livestock (cow, barn, baby-carriage, cup, needle) which makes the UI intuitive for farmers. |
| **Used for** | Tab bar icons, screen headers, action buttons, status indicators |

---

### AsyncStorage (`@react-native-async-storage/async-storage`)
| | |
|---|---|
| **What** | Persistent key-value storage on the device (like localStorage for React Native) |
| **Why** | Enables offline-first features without a database. Used to persist the selected animal across app restarts, store alert preferences, and queue writes when there's no network. Simple API, battle-tested, included in Expo. |
| **Used for** | Selected animal persistence, offline write queue, sent-reminders tracking, alert preferences |

---

## ☁️ Cloud & Backend

### Google Firebase — Firestore (NoSQL Database)
| | |
|---|---|
| **What** | Serverless, scalable NoSQL document database with real-time sync |
| **Why** | **Free tier (Spark plan)** supports up to 50K reads and 20K writes/day — sufficient for 50+ animals. Firestore's document model (collections → documents → fields) maps naturally to livestock data (animals, vaccinations, milk logs). No server management required. Offline persistence is built-in. SDKs available for React Native via `firebase` npm package. |
| **Collections** | `animals`, `sensor_data`, `vaccinations`, `milk_logs`, `pregnancies` |

---

### Adafruit IO (MQTT Broker)
| | |
|---|---|
| **What** | Cloud-hosted MQTT broker with a generous free tier and visual dashboard |
| **Why** | MQTT is the IoT industry standard protocol — lightweight, low-bandwidth, and designed for unreliable networks (QoS levels). Adafruit IO provides 30 data points/minute free, a visual feed dashboard for debugging, and reliable broker uptime. The ESP32 publishes sensor readings to named "feeds" which the app subscribes to in real-time. |
| **Feeds used** | `gpsloc`, `temperature`, `bpm`, `battery`, `geofence`, `humidity` |

---

### Twilio (SMS Alerts)
| | |
|---|---|
| **What** | Cloud communication platform for programmatic SMS |
| **Why** | SMS works on any phone — even basic feature phones — without internet or a smartphone app. Critical for farmers in low-connectivity areas who may not have Expo Go open. Twilio's REST API is called directly from the app when alert thresholds are crossed (high temp, geofence breach, low battery). Cost: ~₹0.50/SMS. |
| **Alerts sent** | Geofence breach, high temperature, high BPM, low battery, vaccination reminders |

---

## 🔌 IoT Hardware

### ESP32 Microcontroller
| | |
|---|---|
| **What** | Dual-core 240MHz microcontroller with built-in WiFi and Bluetooth |
| **Why** | The ESP32 is the de facto standard for IoT projects: powerful enough for MQTT + GPS + sensor reading simultaneously, has built-in WiFi (no separate shield needed), 4MB flash, and costs only ~₹350. Supported by the Arduino IDE and extensive community libraries. |
| **Used for** | Main processing unit for the animal collar device |

---

### NEO-6M GPS Module
| | |
|---|---|
| **What** | UART-based GPS receiver with 2.5m CEP accuracy |
| **Why** | Reliable, widely available, and compatible with ESP32 via SoftwareSerial. Provides NMEA sentences parsed to extract latitude/longitude. Costs ~₹400. Suitable for outdoor livestock tracking where trees and open skies ensure good satellite acquisition. |
| **Used for** | Real-time animal location tracking, geofence breach detection |

---

### DS18B20 Temperature Sensor
| | |
|---|---|
| **What** | 1-Wire digital thermometer with ±0.5°C accuracy |
| **Why** | Waterproof probe version can be attached safely to the animal collar. 1-Wire protocol means only 1 GPIO pin needed on the ESP32. Normal bovine temperature is 38–39.5°C — the app alerts at >40°C, which DS18B20 detects reliably. |
| **Used for** | Body temperature monitoring |

---

### MAX30100 Pulse Oximeter (BPM)
| | |
|---|---|
| **What** | I2C optical heart rate and SpO₂ sensor |
| **Why** | Non-invasive BPM reading via infrared LED — no needles or veterinary expertise needed. Connects via I2C (2 pins). Normal bovine BPM is 60–80; alerts trigger at >100 BPM (indicating stress or fever). |
| **Used for** | Heart rate (BPM) monitoring |

---

### LiPo Battery + Voltage Divider
| | |
|---|---|
| **What** | 3.7V lithium polymer battery with resistor voltage divider to ESP32 ADC pin |
| **Why** | Lightweight, rechargeable, and can power the ESP32 + modules for 12–24 hours. The voltage divider maps battery voltage to 0–3.3V for the ESP32's ADC, giving a battery percentage reading. Alerts fire at <20%. |
| **Used for** | Device power + battery level monitoring |

---

## 🧰 Development Tools

### Node.js + npm
| | |
|---|---|
| **What** | JavaScript runtime and package manager |
| **Why** | Required for running Expo CLI, managing app dependencies, and running the MQTT simulator script. |

---

### `scripts/simulate_data.js` (Custom MQTT Simulator)
| | |
|---|---|
| **What** | Node.js script that publishes realistic sensor data to Adafruit IO via MQTT |
| **Why** | Allows full app testing without physical ESP32 hardware. Supports multiple animals, multiple simulation modes (Normal, Fever, Geofence Alert, Low Battery), and concurrent multi-animal simulation. Invaluable for demonstrating the app at hackathons or demos. |

---

### dotenv (`.env` file)
| | |
|---|---|
| **What** | Loads environment variables from a `.env` file |
| **Why** | Keeps API keys (Adafruit, Twilio, Firebase) out of source code. Expo reads `EXPO_PUBLIC_*` prefixed variables and makes them available to the app at build time. `.env` is in `.gitignore` — credentials are never committed to GitHub. |

---

## 🌐 Localization

### Custom i18n System (`LanguageContext.js` + `translations.js`)
| | |
|---|---|
| **What** | React Context-based translation system with English and Hindi string maps |
| **Why** | No heavy i18n library needed — a lightweight custom hook `useTranslation()` returns `t('key')` translations. Adding a new language (e.g., Marathi) only requires adding a new key block in `translations.js`. Switching language is instant (no app restart). |
| **Languages** | English (default), Hindi (हिन्दी) |

---

## 📄 PDF Generation

### expo-print + expo-sharing
| | |
|---|---|
| **What** | Expo modules to render HTML to a PDF file and share it via the native share sheet |
| **Why** | No external PDF library needed. `expo-print` accepts raw HTML and generates a PDF in the app's cache. `expo-sharing` opens the native "Share" dialog — farmer can WhatsApp the PDF to their vet instantly. Works entirely offline once data is loaded. |
| **Used for** | One-tap comprehensive health + milk + pregnancy PDF report |

---

## 🏗️ Architecture Pattern

### Offline-First (AsyncStorage Queue)
| | |
|---|---|
| **What** | Writes are queued to AsyncStorage when offline and flushed to Firestore when connectivity returns |
| **Why** | Rural India has frequent internet outages. Data entered offline (milk logs, vaccinations) must not be lost. The `offlineService.js` module provides `offlineWrite()` — a drop-in wrapper that handles connectivity transparently. |

### Context API (AnimalContext + LanguageContext)
| | |
|---|---|
| **What** | React's built-in global state management |
| **Why** | The app needs two pieces of truly global state: the selected animal (used by every screen) and the language setting. Context API handles these perfectly without the overhead of Redux or Zustand. |

---

*The stack was chosen to be **affordable, maintainable, and rural-ready** — prioritizing free tiers, offline capability, and low-cost hardware over enterprise complexity.*
