"""
generate_seed_dataset.py
=============================================================
Pashu-Sutra — Calibration Seed Dataset Generator
=============================================================

WORKFLOW:
    Step 1 (this script): Generate a realistic "manually collected"
                          seed Excel sheet — as if a farmer took
                          rectal readings for 10 days while the
                          sensor recorded neck temp simultaneously.

    Step 2 (next script): expand_synthetic.py — use this seed to
                          train a correlation model and generate
                          500–1000 synthetic rows for ML training.

SEED DATA LOGIC:
    - 10 days × 3 readings/day (morning 6am, afternoon 2pm, evening 6pm)
    - For a HEALTHY cow (rectal temp 38.0–39.2°C)
    - Neck surface temp = rectal temp - offset
      offset varies with ambient temp and time of day (real physiology)
    - Farmer manually notes: animal name, date, time, rectal temp, notes
    - Sensor auto-records: neck_temp, ambient_temp (from weather API / DHT11)

HOW TO RUN:
    pip install pandas openpyxl numpy
    python generate_seed_dataset.py

OUTPUT:
    cattle_calibration_seed.xlsx
"""

import numpy as np
import pandas as pd
from datetime import datetime, timedelta

np.random.seed(7)  # reproducible

# ─── Configuration ─────────────────────────────────────────────────
ANIMAL_NAME   = "Nandini"
BREED         = "Gir (Indigenous)"
ANIMAL_ID     = "COW-001"
START_DATE    = datetime(2026, 2, 1)
DAYS          = 10

# 3 readings per day: morning (6:00), afternoon (14:00), evening (18:00)
READINGS = [
    {"label": "Morning",   "hour": 6,  "ambient_range": (18, 24)},
    {"label": "Afternoon", "hour": 14, "ambient_range": (28, 36)},
    {"label": "Evening",   "hour": 18, "ambient_range": (25, 30)},
]

# ─── Physiological Parameters (healthy cow) ───────────────────────
# Rectal temp of healthy bovine: 38.0°C – 39.2°C
# Neck surface runs LOWER than rectal by an "offset"
# offset = BASE - ambient_cooling_effect + diurnal_effect + noise

BASE_OFFSET     = 1.85   # °C (mean literature value)
AMBIENT_COOLING = 0.040  # per °C above 20°C ambient — cold wind = bigger gap
DIURNAL_MORNING = 0.20   # morning: slightly larger offset (cooler neck surface)
DIURNAL_EVENING = 0.10   # evening: moderate offset
DIURNAL_NOON    = 0.05   # peak afternoon: smallest offset (vasodilation in heat)
NOISE_RECTAL    = 0.12   # biological variability in rectal temp (±°C)
NOISE_NECK      = 0.10   # sensor + biological noise in neck temp (±°C)

# ─── Generate Rows ─────────────────────────────────────────────────
rows = []

for day in range(DAYS):
    date = START_DATE + timedelta(days=day)
    date_str = date.strftime("%d-%b-%Y")

    for reading in READINGS:
        hour    = reading["hour"]
        label   = reading["label"]
        amb_lo, amb_hi = reading["ambient_range"]

        # Ambient temperature (realistic range for time of day)
        ambient = round(np.random.uniform(amb_lo, amb_hi), 1)

        # Actual rectal temperature (healthy range, farmer measured)
        rectal = round(np.random.uniform(38.0, 39.2) + np.random.normal(0, NOISE_RECTAL), 2)
        rectal = round(min(max(rectal, 37.8), 39.5), 2)  # clamp

        # Diurnal offset component
        if hour == 6:
            diurnal = DIURNAL_MORNING
        elif hour == 14:
            diurnal = DIURNAL_NOON
        else:
            diurnal = DIURNAL_EVENING

        # Ambient cooling (larger gap when cooler ambient relative to 20°C)
        ambient_effect = AMBIENT_COOLING * max(0, ambient - 20)

        # Neck surface temp = rectal - offset + noise
        offset = BASE_OFFSET - ambient_effect + diurnal
        neck = round(rectal - offset + np.random.normal(0, NOISE_NECK), 2)
        neck = round(min(max(neck, 35.0), 39.0), 2)

        # Farmer notes
        if rectal > 39.0:
            notes = "Slightly elevated — monitor"
        elif hour == 14 and ambient > 34:
            notes = "Hot afternoon, cow in shade"
        else:
            notes = "Normal"

        rows.append({
            "Date":               date_str,
            "Time":               f"{hour:02d}:00",
            "Reading_Session":    label,
            "Animal_Name":        ANIMAL_NAME,
            "Animal_ID":          ANIMAL_ID,
            "Breed":              BREED,
            "Rectal_Temp_C":      rectal,    # ← farmer measured (ground truth)
            "Neck_Temp_C":        neck,      # ← DS18B20 sensor reading
            "Ambient_Temp_C":     ambient,   # ← weather / DHT11
            "Temp_Offset_C":      round(rectal - neck, 2),
            "Farmer_Notes":       notes,
        })

# ─── Build DataFrame ────────────────────────────────────────────────
df = pd.DataFrame(rows)

print("=" * 60)
print("  PASHU-SUTRA CALIBRATION SEED DATASET")
print("=" * 60)
print(df.to_string(index=False))

# ─── Quick Correlation Stats ─────────────────────────────────────────
corr = df["Neck_Temp_C"].corr(df["Rectal_Temp_C"])
avg_offset = df["Temp_Offset_C"].mean()
std_offset = df["Temp_Offset_C"].std()

print(f"\n📊 Quick Statistics:")
print(f"   Total rows           : {len(df)}")
print(f"   Neck-Rectal corr (r) : {corr:.4f}")
print(f"   Avg offset           : {avg_offset:.2f}°C")
print(f"   Offset std dev       : {std_offset:.2f}°C")
print(f"   Avg rectal temp      : {df['Rectal_Temp_C'].mean():.2f}°C")
print(f"   Avg neck temp        : {df['Neck_Temp_C'].mean():.2f}°C")

# ─── Save to Excel with formatting ──────────────────────────────────
output_file = "cattle_calibration_seed.xlsx"
with pd.ExcelWriter(output_file, engine="openpyxl") as writer:

    # Sheet 1: Raw data
    df.to_excel(writer, sheet_name="Calibration_Data", index=False)

    # Sheet 2: Summary stats
    summary = pd.DataFrame({
        "Metric": [
            "Animal Name", "Animal ID", "Breed",
            "Total Readings", "Days Covered",
            "Neck-Rectal Correlation (r)",
            "Average Offset (°C)",
            "Offset Std Dev (°C)",
            "Avg Rectal Temp (°C)",
            "Avg Neck Surface Temp (°C)",
            "Min Rectal", "Max Rectal",
        ],
        "Value": [
            ANIMAL_NAME, ANIMAL_ID, BREED,
            len(df), DAYS,
            f"{corr:.4f}",
            f"{avg_offset:.2f}",
            f"{std_offset:.2f}",
            f"{df['Rectal_Temp_C'].mean():.2f}",
            f"{df['Neck_Temp_C'].mean():.2f}",
            f"{df['Rectal_Temp_C'].min():.2f}",
            f"{df['Rectal_Temp_C'].max():.2f}",
        ]
    })
    summary.to_excel(writer, sheet_name="Summary_Stats", index=False)

    # Sheet 3: Instructions for data generation
    instructions = pd.DataFrame({
        "Step": [1, 2, 3, 4, 5],
        "Action": [
            "Use Calibration_Data sheet as seed rows (30 real paired readings)",
            "Run expand_synthetic.py to generate 500–1000 synthetic rows from this seed",
            "Synthetic rows preserve the discovered offset + correlation pattern",
            "Run train_model.py to fit linear regression on expanded dataset",
            "Embed the resulting coefficients in databaseService.js as estimateCoreTemp()",
        ]
    })
    instructions.to_excel(writer, sheet_name="Next_Steps", index=False)

print(f"\n✅ Excel file saved: {output_file}")
print(f"   Sheets: Calibration_Data | Summary_Stats | Next_Steps")
print(f"\n📌 Next step: run  python expand_synthetic.py")
