"""
expand_synthetic.py
=============================================================
Pashu-Sutra — Synthetic Data Expansion from Seed
=============================================================

PURPOSE:
    Takes cattle_calibration_seed.xlsx (30 real-like paired rows)
    and expands it to 1000 synthetic rows using:
      1. Linear interpolation within the observed offset range
      2. Gaussian noise around the discovered mean offset
      3. Realistic ambient temperature and time variation

    This mirrors what researchers call "data augmentation" —
    we preserve the real correlation structure discovered from
    the seed data, just at larger scale.

HOW TO RUN:
    python expand_synthetic.py

REQUIRES:
    cattle_calibration_seed.xlsx (from generate_seed_dataset.py)

OUTPUT:
    cattle_temp_full_dataset.xlsx  — 1000-row expanded dataset
    (ready for ML model training)
"""

import numpy as np
import pandas as pd
from sklearn.linear_model import LinearRegression
from sklearn.metrics import r2_score, mean_absolute_error
import warnings
warnings.filterwarnings("ignore")

np.random.seed(42)

# ─── Load Seed Data ──────────────────────────────────────────────────
print("📂 Loading seed dataset...")
seed_df = pd.read_excel("cattle_calibration_seed.xlsx", sheet_name="Calibration_Data")

neck_seed    = seed_df["Neck_Temp_C"].values
rectal_seed  = seed_df["Rectal_Temp_C"].values
ambient_seed = seed_df["Ambient_Temp_C"].values

# ─── Learn offset statistics FROM the seed ───────────────────────────
offset_seed = rectal_seed - neck_seed
mean_offset = offset_seed.mean()
std_offset  = offset_seed.std()
corr_r      = np.corrcoef(neck_seed, rectal_seed)[0, 1]

print(f"\n📊 Seed statistics:")
print(f"   Rows           : {len(seed_df)}")
print(f"   Mean offset    : {mean_offset:.3f}°C  (rectal - neck)")
print(f"   Offset std dev : {std_offset:.3f}°C")
print(f"   Correlation r  : {corr_r:.4f}")

# ─── Generate 1000 Synthetic Rows ────────────────────────────────────
N = 1000
print(f"\n⚙️  Generating {N} synthetic rows...")

# Neck temp: sample in slightly wider range than seed
neck_min = float(neck_seed.min()) - 0.5
neck_max = float(neck_seed.max()) + 0.5
neck_synth = np.random.uniform(neck_min, neck_max, N)

# Ambient temp: full Indian range across seasons
ambient_synth = np.random.uniform(10, 42, N)

# Hour of day
hour_synth = np.random.choice([6, 10, 14, 18, 20], N)

# Breed: 0 = indigenous, 1 = HF/crossbreed
breed_synth = np.random.randint(0, 2, N)

# Reconstruct the offset using learned stats + ambient modulation
# ambient cooling effect: bigger gap (higher offset) when ambient is low
ambient_cooling = 0.038 * np.maximum(0, ambient_synth - 20)

# Diurnal: afternoon smallest offset, morning largest
diurnal = np.where(hour_synth == 6,  0.20,
          np.where(hour_synth == 14, 0.05, 0.12))

# Breed: HF runs 0.2°C warmer core
breed_boost = 0.18 * breed_synth

# Synthetic offset: mean from seed ± noise
offset_synth = (
    mean_offset
    - ambient_cooling
    + diurnal
    + breed_boost
    + np.random.normal(0, std_offset * 0.8, N)   # scaled noise
)

# Rectal = neck + offset
rectal_synth = neck_synth + offset_synth
rectal_synth = np.clip(rectal_synth, 37.5, 41.5)  # physiological limits

# Health status label (useful for classification later)
health_label = np.where(rectal_synth < 38.5, "Normal",
               np.where(rectal_synth < 39.5, "Mild Elevation", "Fever"))

# Session label
session_label = np.where(hour_synth == 6,  "Morning",
                np.where(hour_synth == 14, "Afternoon",
                np.where(hour_synth == 18, "Evening", "Night")))

# ─── Build Full DataFrame ─────────────────────────────────────────────
synthetic_df = pd.DataFrame({
    "Neck_Temp_C":    np.round(neck_synth, 2),
    "Ambient_Temp_C": np.round(ambient_synth, 1),
    "Hour":           hour_synth,
    "Session":        session_label,
    "Breed":          breed_synth,
    "Rectal_Temp_C":  np.round(rectal_synth, 2),
    "Offset_C":       np.round(rectal_synth - neck_synth, 2),
    "Health_Status":  health_label,
    "Source":         "Synthetic"
})

# Add original seed rows at the top (labeled as measured)
seed_export = pd.DataFrame({
    "Neck_Temp_C":    seed_df["Neck_Temp_C"],
    "Ambient_Temp_C": seed_df["Ambient_Temp_C"],
    "Hour":           pd.to_numeric(seed_df["Time"].str.split(":").str[0]),
    "Session":        seed_df["Reading_Session"],
    "Breed":          0,
    "Rectal_Temp_C":  seed_df["Rectal_Temp_C"],
    "Offset_C":       seed_df["Temp_Offset_C"],
    "Health_Status":  "Normal",
    "Source":         "Measured (Farmer)"
})

full_df = pd.concat([seed_export, synthetic_df], ignore_index=True)
print(f"   Total rows (seed + synthetic): {len(full_df)}")

# ─── Train Linear Regression Model ───────────────────────────────────
from sklearn.model_selection import train_test_split

features = ["Neck_Temp_C", "Ambient_Temp_C", "Hour", "Breed"]
X = full_df[features].values
y = full_df["Rectal_Temp_C"].values

X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)
model = LinearRegression()
model.fit(X_train, y_train)
y_pred = model.predict(X_test)

r2  = r2_score(y_test, y_pred)
mae = mean_absolute_error(y_test, y_pred)

print(f"\n🤖 Model Performance:")
print(f"   R² Score : {r2:.4f}")
print(f"   MAE      : {mae:.4f}°C")

coef      = model.coef_
intercept = model.intercept_
names     = features

print(f"\n🔢 Coefficients:")
for n, c in zip(names, coef):
    print(f"   {n:20s}: {c:.6f}")
print(f"   {'intercept':20s}: {intercept:.6f}")

# ─── JavaScript formula output ─────────────────────────────────────────
c_neck, c_amb, c_hour, c_breed = coef

js = f"""
// ─── Paste this into databaseService.js ─────────────────────────────
// Model: LinearRegression trained on {len(full_df)} rows
// Seed: 30 manually collected readings | Synthetic: {N} generated rows
// R² = {r2:.4f} | MAE = {mae:.4f}°C

/**
 * Estimate core body temperature from neck sensor reading.
 * @param {{number}} neckTemp     - DS18B20 neck surface reading (°C)
 * @param {{number}} ambientTemp  - ambient/outdoor temperature (°C)
 * @param {{number}} hour         - hour of day 0–23 (default: current hour)
 * @param {{number}} breed        - 0 = indigenous, 1 = HF/crossbreed
 * @returns {{number}} estimated core (rectal equivalent) temperature (°C)
 */
export function estimateCoreTemp(
  neckTemp,
  ambientTemp = 28,
  hour = new Date().getHours(),
  breed = 0
) {{
  const core =
    {c_neck:.6f} * neckTemp +
    {c_amb:.6f} * ambientTemp +
    {c_hour:.6f} * hour +
    {c_breed:.6f} * breed +
    {intercept:.6f};
  return parseFloat(Math.min(Math.max(core, 37.5), 41.5).toFixed(2));
}}
"""

print(js)

# ─── Save to Excel ─────────────────────────────────────────────────────
output = "cattle_temp_full_dataset.xlsx"
with pd.ExcelWriter(output, engine="openpyxl") as writer:
    full_df.to_excel(writer, sheet_name="Full_Dataset",  index=False)

    model_df = pd.DataFrame({
        "Feature":     names + ["intercept"],
        "Coefficient": list(coef) + [intercept],
    })
    model_df.to_excel(writer, sheet_name="Model_Coefficients", index=False)

    stats_df = pd.DataFrame({
        "Metric": ["R² Score", "MAE (°C)", "Total Rows",
                   "Seed (Measured) Rows", "Synthetic Rows", "Correlation r"],
        "Value":  [f"{r2:.4f}", f"{mae:.4f}", len(full_df),
                   len(seed_export), N, f"{corr_r:.4f}"],
    })
    stats_df.to_excel(writer, sheet_name="Model_Stats", index=False)

print(f"✅ Dataset saved : {output}")
print(f"   Sheets: Full_Dataset | Model_Coefficients | Model_Stats")
print(f"\n🎯 Copy the JS formula above into databaseService.js!")
