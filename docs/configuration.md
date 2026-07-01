# Configuration & Calibration

## Important User-Configurable Values

These directly affect displayed speed, SOC, efficiency, and range estimates.

### Mechanical
- `wheel_diameter_mm` — measure actual rolling diameter of your tire (not just nominal). Best done with tape on the ground.
- `motor_pole_pairs` — number of pole pairs (not poles). Common:
  - Many hub motors: 14-20 poles → 7-10 pairs
  - Mid-drives vary
- `gear_ratio` — motor revolutions per wheel revolution. 1.0 = direct. Belt/chain = teeth_wheel / teeth_motor.

### Battery
- `battery_s` — number of cells in series
- `battery_max_v` and `battery_min_v` — for simple voltage-based SOC. Better to use actual BMS SOC when available.
- For more accuracy you can implement a discharge curve or coulomb counting (using Ah counters from VESC + known capacity).

### Other
- Units: metric (km/h, km, °C) vs imperial (mph, miles, °F)
- Update rate
- Display brightness defaults
- Power mode mappings (these usually map to different current limits or speed limits on the VESC side via app config or Lisp)

## How to Set Them

**Initial approach (firmware constants)**
Edit in `main.cpp` or a `config.h`.

**Better (recommended)**
- Store in ESP32 `Preferences` (NVS) or LittleFS JSON
- Expose via BLE "Config" characteristic so the mobile app can read/write them
- Provide sensible defaults + a calibration screen in the app

## Speed Verification

1. Connect phone to ESP32 BLE (or use serial monitor)
2. Spin the wheel at a known speed (use a drill + tachometer or GPS on flat road)
3. Compare displayed speed vs actual
4. Adjust diameter or pole pairs until it matches

Tip: VESC "rpm" field is usually electrical RPM (eRPM). The formula in the skeleton accounts for pole pairs.

## Trip / Odometer

- Trip: resetable (button on screen or from phone)
- Odo: lifetime (or per tire/belt if you want fancy maintenance reminders)
- Both should survive power loss

Use `Preferences` library:

```cpp
preferences.begin("minibike", false);
float odo = preferences.getFloat("odo", 0.0f);
...
preferences.putFloat("odo", newOdo);
```

## BMS vs Voltage SOC

If you have a VESC-compatible BMS on CAN, prefer its SOC value. Fall back to voltage-based only when needed.

## Power Modes

The local screen and mobile app should be able to request different modes.

Common implementation:
- The ESP32 tells VESC to change limits, or
- You have pre-configured "App" profiles in VESC Tool and the display just shows the active one (via CAN or status)

## Next

Add a `Config` struct + load/save functions, then wire it into the BLE config characteristic and the Flutter settings screen.

**For the full story** (formulas, calibration steps, verification, common mistakes, example configs) read the much more detailed guide:

→ **[docs/stat-configuration-guide.md](stat-configuration-guide.md)**
