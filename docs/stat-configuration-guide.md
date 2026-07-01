# MiniBike Stats — Detailed Stat Readout Configuration Guide

This guide explains **every major telemetry value** shown on the local screen and mobile app, how it is calculated, and how to configure and calibrate it for accurate real-world results.

Accurate stats are critical for safety (don't over-discharge your battery or overheat the motor/controller) and for enjoyment (know your efficiency and range).

## 1. VESC Telemetry Basics (What the Controller Actually Reports)

The ESP32 primarily pulls data using `COMM_GET_VALUES` (via UART or CAN) from the VESC. Key raw fields (from `VescUart` or equivalent):

| Raw Field              | Description                                      | Typical Units |
|------------------------|--------------------------------------------------|---------------|
| `inpVoltage`           | Battery voltage under load                       | Volts         |
| `avgInputCurrent`      | Current drawn from battery                       | Amps          |
| `avgMotorCurrent`      | Current through motor phases (higher than input) | Amps          |
| `dutyCycleNow`         | Throttle / PWM duty (0–1.0)                      | fraction      |
| `rpm`                  | Electrical RPM (eRPM)                            | eRPM          |
| `tempMosfet` / `tempMotor` | Temperatures                                 | °C            |
| `ampHours`, `wattHours` | Cumulative energy counters (drawn)             | Ah / Wh       |
| `tachometer` / `tachometerAbs` | Wheel encoder-like counts (signed / absolute) | counts     |

**Important**: VESC "rpm" is almost always **electrical RPM**, not mechanical wheel RPM.

## 2. Speed Calculation & Calibration (The Most Common Source of Error)

### Formula Used

```cpp
float motor_rpm = erpm / (pole_pairs * 2.0f);   // mechanical motor RPM
float wheel_rpm = motor_rpm / gear_ratio;
float circumference_m = (wheel_diameter_mm / 1000.0f) * PI;
float speed_ms = wheel_rpm * circumference_m / 60.0f;
speed_kmh = speed_ms * 3.6f;
```

### Config Values (in firmware or via app)

- **`WHEEL_DIAMETER_MM`** — **Most important**. Do **not** use the marketing number on the tire sidewall.
  - Recommended method: Roll the bike in a straight line on flat ground. Mark start and end of **one full wheel revolution**. Measure with a tape measure. Do it 3 times and average.
  - Example: Nominal 16" tire might actually roll at 380–410 mm depending on pressure and load.

- **`MOTOR_POLE_PAIRS`**
  - Pole pairs = total poles / 2
  - Hub motors: commonly 7–10 pole pairs (14–20 poles)
  - How to find: Look in VESC Tool → Motor → "Number of poles", or measure by spinning by hand and counting electrical cycles with a scope/multimeter on one phase while turning the wheel slowly.

- **`GEAR_RATIO`**
  - 1.0 = direct drive (most hub motors)
  - For mid-drive or belt/chain: (teeth on wheel sprocket / teeth on motor sprocket) or measured ratio.

### Calibration Procedure (Do This!)

1. Put the bike on a stand or have a friend hold it.
2. Use a GPS speed app on your phone or a bicycle speed sensor as ground truth.
3. Ride at steady speeds (10, 20, 30 km/h).
4. Compare displayed speed vs real.
5. Adjust `WHEEL_DIAMETER_MM` first (small changes have big effect).
6. If speed is consistently off by a fixed ratio, adjust pole pairs or gear ratio.

**Pro tip**: Tire pressure changes diameter significantly. Calibrate at your normal riding pressure.

## 3. Battery State of Charge (SOC)

Two main methods are supported:

### A. Voltage-based (default, simple)

```cpp
soc = (v_in - BATTERY_MIN_V) / (BATTERY_MAX_V - BATTERY_MIN_V) * 100.0;
```

Config:
- `BATTERY_S` (series cells) — helps sanity check (e.g. 13s Li-ion)
- `BATTERY_MAX_V` — 4.2V × S for fully charged Li-ion (13 × 4.2 = 54.6V)
- `BATTERY_MIN_V` — usually 3.0V × S or the VESC low voltage cutoff

**Limitations**: Voltage sags under load. SOC will look lower when accelerating hard and recover when you stop.

### B. Better Methods (Recommended)

- Use a VESC BMS over CAN → read real `soc` from BMS.
- Coulomb counting: Use `ampHours` + known battery capacity.
- Hybrid: voltage at rest + coulomb counting while riding.

Future enhancement: Store full discharge curve in config.

## 4. Trip Distance, Odometer & Tachometer

VESC provides `tachometer` (relative) and `tachometerAbs` (accumulated).

### Conversion to Distance

```cpp
float distance_per_count = (WHEEL_DIAMETER_MM / 1000.0f * PI) / (MOTOR_POLE_PAIRS * 2 * GEAR_RATIO);
// Then multiply by delta tachometer counts
```

### Best Practices

- **Trip**: Resettable (from screen buttons or phone). Reset at start of every ride or day.
- **Odometer**: Persistent across reboots. Stored in ESP32 `Preferences` (NVS).
- Keep a "maintenance odo" for tires, belts, brakes if desired (advanced).

**Common issue**: Tachometer can jump or go backward during regen or when the controller is off. Use `tachometerAbs` for odo and filter deltas for trip.

## 5. Power, Energy & Efficiency

### Instant Power
```cpp
power_w = v_in * i_in;           // Battery power (most useful)
motor_power = v_in * i_motor;    // Sometimes higher due to efficiency
```

### Energy Counters
- `ampHours` / `wattHours` from VESC are cumulative since last reset (or power cycle in some firmwares).
- For **trip** energy: Record starting counters when trip is reset, then subtract.

### Efficiency
```cpp
efficiency_wh_per_km = (wh_used / trip_km);
range_estimate_km = (remaining_wh / wh_per_km);
```

Display both `Wh/km` (lower is better) and `km/kWh`.

## 6. Temperatures & Thermal Protection

- `tempMosfet` (or multiple FET temps on some VESCs)
- `tempMotor` (requires motor temp sensor — highly recommended!)

The screen/app should change colors:
- < 60°C → Green
- 60–85°C → Yellow/Amber
- > 85–90°C → Red + warning

Configure thermal limits in **VESC Tool** (Motor → Limits). The display just shows the data.

## 7. Full Recommended Config Structure

Move beyond `#define` to a proper struct + persistence:

```cpp
struct BikeConfig {
  float wheel_diameter_mm = 400.0f;
  uint8_t pole_pairs = 7;
  float gear_ratio = 1.0f;
  float battery_max_v = 54.6f;
  float battery_min_v = 39.0f;
  uint8_t battery_s = 13;
  bool use_imperial = false;
  uint8_t current_skin = 0;     // for UI skins
  // ...
};
```

Load/save using `Preferences` library.

Expose via BLE Config characteristic so the mobile app can edit + save.

## 8. VESC Tool Settings That Affect Telemetry Quality

Go into VESC Tool and verify:

- Motor → General → Number of poles (must match your `pole_pairs * 2`)
- Motor → Sensors → Correct sensor type (if using hall/encoder)
- App → General → Correct "App to Use" (UART or CAN)
- Limits → Battery cutoffs (affects when SOC hits 0)
- CAN bus settings if using multiple devices (BMS, multiple ESCs)

## 9. Verification Checklist

| Stat          | How to Verify                              | Tool to Use              |
|---------------|--------------------------------------------|--------------------------|
| Speed         | GPS app or calibrated bike computer        | Phone + stand test       |
| SOC           | Resting voltage vs multimeter + known %    | Multimeter + VESC Tool   |
| Trip Distance | Measure a known loop with GPS or tape      | GPS track                |
| Power         | Compare to clamp meter on battery leads    | DC clamp meter           |
| Temps         | IR thermometer on FETs / motor housing     | IR gun                   |
| Efficiency    | Full charge → full discharge on known route| Wh used / km             |

## 10. Example Configurations

**Typical 13s Hub Motor Minibike**
```cpp
WHEEL_DIAMETER_MM = 390
MOTOR_POLE_PAIRS = 8
GEAR_RATIO = 1.0
BATTERY_MAX_V = 54.6
BATTERY_MIN_V = 39.0
```

**Mid-Drive with Chain**
```cpp
WHEEL_DIAMETER_MM = 520   // 20" wheel example
MOTOR_POLE_PAIRS = 4
GEAR_RATIO = 3.5          // 42T wheel / 12T motor example
```

## 11. Future Improvements (Roadmap Items)

- Full coulomb + voltage hybrid SOC
- Automatic wheel diameter learning from GPS (mobile app)
- Per-ride profiles (commute vs fun vs hill)
- Maintenance reminders based on odo

---

**Next Steps After Reading This Guide**

1. Measure your real wheel diameter.
2. Confirm pole pairs in VESC Tool.
3. Set correct constants in firmware (or later via app).
4. Do the calibration rides.
5. Add a proper `Config` struct + NVS storage.

See also:
- `docs/configuration.md` (shorter reference)
- `docs/protocol.md` (how config travels over BLE)
- `firmware/src/main.cpp` (current implementation)

If you have a specific motor/controller + wheel size, share the details and we can give you exact starting numbers.
