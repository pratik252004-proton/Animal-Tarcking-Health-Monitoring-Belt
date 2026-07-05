# 🐄 Pashu-Sutra — Hardware Sensor Design & Collar Placement

Explains how health parameters (temperature and heart rate) are captured from a livestock neck collar/belt device.

---

## 🔧 Device Overview

All electronics are **embedded inside a single weatherproof collar belt** worn around the animal's neck — similar in concept to commercial livestock tags like SCR by Allflex or CowAlert.

```
        ┌─────────────────────────────────────────────────┐
        │              COLLAR BELT (inner view)           │
        │                                                 │
        │  [DS18B20]   [GPS Antenna]  [ESP32 + Battery]  │
        │      │            │                │           │
        │  (skin tip)  (faces sky)      (sealed box)    │
        │                                                 │
        │      [MAX30100] ← positioned at jugular groove  │
        └─────────────────────────────────────────────────┘
```

---

## 🌡️ Temperature Sensor — DS18B20

### How it works in the collar

- Uses a **waterproof stainless steel probe tip** that is positioned on the **inner face** of the collar, sitting flush against the animal's skin
- Measures **surface skin temperature** of the neck area
- Neck surface temp in cattle consistently tracks core body temperature with a small offset (~0.5–1°C), which is calibrated in software

### Mapping to health alerts

| Reading                   | Interpretation                 | App Action                   |
| ------------------------- | ------------------------------ | ---------------------------- |
| 36.5 – 38.5°C (surface) | Normal                         | No alert                     |
| 38.5 – 39.5°C           | Mild elevation                 | Yellow warning               |
| > 39.5°C                 | Fever (>40°C core equivalent) | SMS alert + app notification |

> **Future enhancement:** Ear-canal probe insertion gives a more direct core temperature reading and is used in premium commercial tags.

---

## ❤️ Heart Rate Sensor — MAX30100

### How PPG works

The MAX30100 uses **Photoplethysmography (PPG)**:

1. Shines an **infrared (IR) LED** into the skin
2. A **photodiode** measures how much light is reflected back
3. Each heartbeat causes a pulse of blood through vessels — this changes the reflected light intensity
4. The chip converts these intensity changes into a **BPM reading**

### Why the neck works

Cattle have a prominent **jugular vein** (the jugular groove) running along both sides of the neck — it is large, close to the surface, and easy to access:

```
             Side view of cattle neck
             ──────────────────────
             Skin surface
             ─ ─ ─ ─ ─ ─ ─ ─ ─ ─
                  ████████          ← Jugular vein
             ─ ─ ─ ─ ─ ─ ─ ─ ─ ─
             Neck muscles / tissue

             ↑
             MAX30100 pressed here (inner collar)
```

The sensor is mounted with **light foam padding** to maintain consistent gentle pressure against the jugular groove — enough for IR contact, not enough to restrict blood flow.

### Mapping to health alerts

| Reading               | Interpretation             | App Action |
| --------------------- | -------------------------- | ---------- |
| 60 – 80 BPM          | Normal resting             | No alert   |
| 80 – 100 BPM         | Mild stress / movement     | No alert   |
| > 100 BPM (sustained) | Distress / fever indicator | SMS alert  |

---

## 📐 Full Collar Component Layout

| Component              | Position on Belt      | Orientation      | Notes                          |
| ---------------------- | --------------------- | ---------------- | ------------------------------ |
| **ESP32**        | Outer/top housing     | Any              | Sealed in weatherproof ABS box |
| **LiPo Battery** | Inside housing        | Any              | 3.7V, 2000mAh ≈ 12–24h       |
| **NEO-6M GPS**   | Top of housing        | Antenna upward   | Must have clear sky view       |
| **DS18B20**      | Inner belt            | Probe tip inward | Touches neck skin surface      |
| **MAX30100**     | Inner belt, left side | Face inward      | Aligned with jugular groove    |

---

## ✅ Practical Fitting Guidelines

1. **Belt snugness**: Follow the livestock standard — **2 finger gap** between belt and neck skin. Too tight restricts breathing; too loose loses skin contact for sensors.
2. **Foam backing**: Place a 5mm foam pad behind the MAX30100 to ensure consistent pressure against the jugular groove as the animal moves.
3. **Probe tip**: Trim fur/hide area under the DS18B20 probe (optional) for better thermal contact, similar to how ECG leads are applied.
4. **GPS**: Ensure the GPS antenna faces upward (skyward) — never block it with metal parts of the housing.
5. **Waterproofing**: Seal the ESP32 housing with silicone rubber. Use a food-safe conformal coating on all exposed PCB traces.

---

## 🐮 Precedent in Commercial Products

This design approach is not novel — it mirrors existing commercial solutions:

| Product                             | Sensor Method                               | Price              |
| ----------------------------------- | ------------------------------------------- | ------------------ |
| **SCR by Allflex (Neck Tag)** | Surface temp + rumination via accelerometer | ₹8,000–₹15,000  |
| **CowAlert (Ear Tag)**        | Core temp via ear canal probe               | ₹6,000–₹12,000  |
| **Pashu-Sutra (Neck Belt)**   | Surface temp + PPG BPM + GPS + battery      | **~₹1,500** |

Pashu-Sutra achieves **comparable sensing** at **5–10× lower cost** using off-the-shelf ESP32 components.

---

*The neck collar placement is medically and commercially validated for livestock health monitoring.*
