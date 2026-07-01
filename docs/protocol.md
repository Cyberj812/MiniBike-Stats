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

## Telemetry Packet (v1) - Currently Implemented

The firmware currently sends this exact struct over the telemetry characteristic (little-endian).

```c
// firmware/src/main.cpp
struct __attribute__((packed)) TelemetryPacketV1 {
  uint8_t  version = 1;
  uint32_t ts_ms;

  float    v_in;
  float    i_in;
  float    i_motor;
  float    duty;
  float    rpm;            // eRPM
  float    temp_mos;
  float    temp_motor;
  uint8_t  fault;

  float    speed_kmh;
  float    power_w;
  float    soc;
  float    trip_km;

  uint8_t  power_mode;
  uint8_t  lights;
  uint8_t  cruise;
};
```

Dart parser in `mobile/lib/models/telemetry_packet.dart` matches this byte-for-byte.

**Notes**:
- All floats are IEEE 754 single precision.
- Version byte at front allows future evolution without breaking clients.
- App can ignore unknown higher-version fields or request full config.

## Commands (Write) - Currently Implemented

Single-byte commands sent to the command characteristic.

| Byte | Command          | Description                     |
|------|------------------|---------------------------------|
| 0x01 | CYCLE_SKIN       | Switch the on-bike display skin |
| 0x02 | RESET_TRIP       | Zero the trip distance          |

More commands (power mode, lights, etc.) will be added as features are built.

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
See the working implementation:
- `mobile/lib/services/minibike_ble.dart`
- `mobile/lib/models/telemetry_packet.dart`

It uses `flutter_blue_plus` and subscribes with `setNotifyValue(true)`.
