# Mobile App (Flutter)

This is the companion phone application for the minibike ESP32 dashboard.

**Real-time Bluetooth communication is fully implemented.**

The phone connects to the ESP32 over BLE and receives live telemetry at ~5 Hz.

## Current BLE Features

- Automatic scan for device named `MiniBikeStats`
- Real-time telemetry notifications (speed, voltage, currents, temps, SOC, power, trip, etc.)
- Send commands back to the bike:
  - Cycle the on-bike display skin
  - Reset trip distance
- Clean packet parsing that exactly matches the ESP32 `TelemetryPacketV1` struct
- Connection state management + auto-reconnect ready

## Getting Started

```bash
cd mobile

# Get dependencies
flutter pub get

# Run on your phone (Android or iOS)
flutter run
```

### Important Notes

- **Android**: Location permission + Bluetooth permissions are required (the app will prompt).
- **iOS**: Bluetooth permission is requested automatically.
- Make sure the ESP32 is powered and the firmware is flashed (it advertises as `MiniBikeStats`).

## Project Structure

```
mobile/
├── lib/
│   ├── main.dart                 # App entry + live dashboard
│   ├── models/telemetry_packet.dart   # Exact match to ESP32 struct
│   ├── services/minibike_ble.dart     # BLE manager (connect, notify, commands)
│   └── themes/skins.dart         # UI skin definitions
├── pubspec.yaml
└── README.md
```

## Architecture

```
ESP32 (NimBLE)  <--- BLE Notify (200ms) --->  Flutter (flutter_blue_plus)
     │                                              │
     ├── TelemetryPacketV1 (binary)                 ├── MiniBikeBle (ChangeNotifier)
     └── Command byte (0x01, 0x02, ...)             └── Dashboard updates in real time
```

See:
- `docs/protocol.md` for the protocol
- Firmware `src/main.cpp` for the ESP32 implementation

## Future Work

- Add the other skins (Analog, Minimal, Race) visually in the Flutter UI
- Ride logging + history
- Settings screen for calibration values (pushed to ESP32)
- Graphs using fl_chart
- Better error handling and reconnection logic

Run `flutter pub get` and connect your phone to the ESP32 to see live stats!
