# Telemetry Protocol (BLE)

This document defines the Bluetooth Low Energy interface between the ESP32 dashboard and the mobile app.

## Philosophy
- Primary transport: BLE (low power, works great for riders)
- Realtime data via **Notify** (fast updates, no polling)
- Commands via **Write** (power modes, lights, trip reset, etc.)
- Keep payloads small and versioned for compatibility
- Optional future: Wi-Fi WebSocket or mDNS for desktop tools

## BLE Service & Characteristics

**Service UUID**: `0xABCD` (or full 128-bit: `a1b2c3d4-0000-0000-0000-00000000beef` — we'll pick one)

### Characteristics

| UUID (short) | Name                  | Properties     | Description |
|--------------|-----------------------|----------------|-------------|
| 0x1001       | Telemetry Notify      | Notify         | Packed realtime data, sent periodically (~5-10 Hz) |
| 0x1002       | Commands              | Write, WriteNR | Send control commands from phone |
| 0x1003       | Config                | Read/Write     | Get/set persistent config (wheel size, battery params, units) |
| 0x1004       | Status / Faults       | Notify         | Faults, warnings, state machine |
| 0x1005       | Ride Log Chunk        | Notify         | (Future) historical ride chunks if stored on ESP |

Use 128-bit UUIDs in practice for production to avoid collisions.

## Telemetry Packet (v1)

Sent as a compact binary struct (little-endian). Notify payload ≤ 20-180 bytes typical (MTU dependent).

Proposed packed struct (C):

```c
typedef struct __attribute__((packed)) {
    uint8_t  version;           // 1
    uint32_t timestamp_ms;      // uptime or millis

    // Core VESC
    float    v_in;              // V
    float    i_in;              // A (battery)
    float    i_motor;           // A
    float    duty;              // 0.0 - 1.0
    float    rpm;               // eRPM
    float    temp_mos;          // °C
    float    temp_motor;        // °C
    uint8_t  fault;             // mc_fault_code

    // Energy & distance
    float    ah_used;           // cumulative or trip?
    float    wh_used;
    int32_t  tachometer;        // current
    int32_t  tachometer_abs;    // lifetime

    // Derived (pre-computed on ESP where possible)
    float    speed_kmh;         // or mph flag in config
    float    power_w;           // v_in * i_in or motor power
    float    soc_pct;           // 0-100 (voltage or BMS)
    float    trip_km;           // resettable
    float    efficiency_whkm;   // last segment or avg

    // Status flags
    uint8_t  power_mode;        // 0=eco, 1=normal, 2=sport, ...
    uint8_t  lights;            // bitmask
    uint8_t  cruise;            // 0/1
    uint8_t  charging;          // 0/1
} telemetry_v1_t;
```

**Notes**:
- All floats are IEEE 754 single precision.
- Version byte at front allows future evolution without breaking clients.
- App can ignore unknown higher-version fields or request full config.

## Commands (Write)

Simple TLV or fixed small command packets.

Examples:

- `CMD_SET_POWER_MODE` + uint8_t mode
- `CMD_LIGHTS_TOGGLE` / `CMD_LIGHTS_SET` + mask
- `CMD_CRUISE_SET` + on/off
- `CMD_RESET_TRIP`
- `CMD_BEEP` (for finding the bike)

Define in `firmware/src/protocol.h` and mirror in Dart.

## Config Sync

Read/Write a JSON or packed struct containing:
- wheel_diameter_mm
- motor_pole_pairs
- gear_ratio (1.0 = direct)
- battery_s (series count)
- battery_min_v, battery_max_v (for SOC)
- units (0=metric, 1=imperial)
- update_rate_hz
- display_brightness
- etc.

## Future / Advanced
- Larger MTU negotiation for bigger packets
- Multiple telemetry profiles (basic vs full)
- Protobuf for complex future payloads
- Authentication / bonding for command channel (important if controlling throttle limits)

## Implementation Tips

**ESP32 (NimBLE or ArduinoBLE)**
- Use a single characteristic for telemetry with `pCharacteristic->notify()`
- Send at fixed interval or on significant change + max rate throttle
- Keep MTU high (`BLEDevice::setMTU(185)` or negotiate)

**Flutter**
```dart
final subscription = telemetryChar.onValueReceived.listen((value) {
  final data = parseTelemetry(value);
  // update UI
});
await telemetryChar.setNotifyValue(true);
```

See `firmware/src/` for reference implementation (coming soon).
