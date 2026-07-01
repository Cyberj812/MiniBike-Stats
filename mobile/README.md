# Mobile App (Flutter)

This is the companion phone application for the minibike ESP32 dashboard.

## Planned Features (inspired by VESC Dash + more)

- Beautiful realtime dashboard (big speed, battery, power, temps)
- Multiple views: main, trip/stats, charts, BMS
- Live updating graphs (power, speed, voltage over time)
- Ride recording + history (local database)
- Export (CSV, GPX)
- Controls: power mode, lights, cruise request, trip reset
- Settings & calibration: wheel size, battery params, units, connection
- Optional: maps (if GPS on bike or phone), weather, etc.

## Getting Started

```bash
# From the project root
cd mobile

# Create the Flutter project (if not already scaffolded)
flutter create --platforms android,ios .
# or flutter create minibike_dash then move files

flutter pub add flutter_blue_plus fl_chart path_provider shared_preferences

# Then run
flutter run
```

## BLE Integration

- Scan for "MinibikeDash"
- Connect
- Discover service `a1b2c3d4-0000-0000-0000-00000000beef`
- Subscribe to telemetry characteristic
- Parse the packed `TelemetryPacketV1` struct (see ../docs/protocol.md)
- Send commands on the command characteristic

Example packages:
- `flutter_blue_plus` (recommended current choice)
- For structured data you can use `protobuf` later

## State Management

Recommended: Riverpod or Provider + Freezed models for the telemetry object.

## UI Inspiration

- Large, high-contrast numbers for riding
- Dark theme by default
- Swipeable pages or bottom nav
- Optional analog speedo style

See root README and docs/ for the full vision.
