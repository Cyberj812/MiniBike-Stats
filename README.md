# MiniBike Stats

[![GitHub](https://img.shields.io/badge/GitHub-Cyberj812%2FMiniBike--Stats-blue?logo=github)](https://github.com/Cyberj812/MiniBike-Stats)

Open-source telemetry system for an electric minibike using ESP32 + custom display, with a companion mobile app.

Inspired by commercial solutions like the [VESC Dash 35B](https://www.vesclabs.com/product/vesc-dash-35b/), this project provides real-time stats, trip data, controls, and logging — but fully customizable with your own hardware and a phone app for richer visualization and ride history.

## Goals

- Local handlebar-mounted screen (your choice of display) driven by ESP32
- Mobile app (iOS + Android) for detailed stats, graphs, controls, and ride logging
- Works with VESC (or similar) motor controllers
- Extendable to BMS, lights, power modes, cruise, etc.
- Low cost, hackable, open source

## Inspiration / Stats Like the VESC Dash 35B

The commercial VESC Dash shows:
- Real-time telemetry: speed, battery voltage/current/power, motor & controller temps, duty cycle
- Trip & totals: distance (trip + odometer), energy consumption (Ah/Wh), efficiency
- Vehicle status: lights, indicators, cruise, power modes
- BMS data (cell voltages, SOC when available)
- Multiple pages + tactile button navigation
- Wi-Fi + BLE connectivity (can even act as bridge)

This project aims for similar (or better) visibility on your custom screen **plus** a full-featured phone app.

## High-Level Architecture

```mermaid
flowchart TD
    VESC[VESC Motor Controller] <-->|UART or CAN| ESP32[ESP32]
    BMS[BMS / other CAN nodes] <-->|CAN| ESP32
    ESP32 -->|SPI / RGB / etc.| Display[Custom TFT / LCD / Touch Screen]
    ESP32 -->|BLE (primary)<br/>or Wi-Fi| Phone[Mobile App]
    ESP32 -->|Optional| SD[SD Card Logging]
    ESP32 -->|GPIO| Lights[Lights, Horn, Buttons]
    Phone --> Graphs[Charts, History, Export]
```

- **ESP32** is the central brain: reads telemetry, drives the screen, serves data over BLE, handles buttons, computes derived values (speed, SOC, efficiency, trip counters).
- **Mobile app** connects via BLE to receive streamed telemetry and send commands.
- Persistent state (odo, trip, config) on ESP32 (EEPROM / Preferences / LittleFS) or mirrored to phone.

## Recommended Hardware

### ESP32
- ESP32-S3 recommended (more RAM, PSRAM options, better for LVGL + BLE + compute)
- Alternatives: ESP32-C3 (like the commercial dash), regular ESP32

### Display Options (pick one that fits your handlebar enclosure)
- Waveshare ESP32-S3 3.5" or 4.3" capacitive touch LCDs (excellent LVGL support)
- Elecrow 5" ESP32 HMI displays (RGB parallel, very nice)
- Cheap ILI9341 / ST7789 3.5" 320x480 SPI + standalone ESP32-S3
- Any LVGL-capable board you like (outdoor brightness a plus)

### Connectivity to VESC
- **Preferred**: CAN bus (robust, supports multiple devices like BMS)
- **Easy start**: UART (115200 baud, many libraries, single VESC)

### Other
- 5V or 12V regulated power from the bike pack (good filtering + protection)
- Buttons or rotary encoder for local UI navigation
- Optional: GPS (for true speed / maps), IMU, SD card, buzzer

## Core Telemetry Fields (VESC)

Primary data from VESC `get_values()` / status packets:

| Field                  | Unit     | Notes |
|------------------------|----------|-------|
| inpVoltage             | V        | Battery voltage |
| avgInputCurrent        | A        | Battery current |
| avgMotorCurrent        | A        | Motor phase current |
| dutyCycleNow           | %        | 0-100 |
| rpm                    | eRPM     | Convert to wheel speed |
| tempMosfet             | °C       | Controller temp |
| tempMotor              | °C       | Motor temp (if sensor present) |
| ampHours / wattHours   | Ah / Wh  | Cumulative |
| tachometer / Abs       | counts   | For distance |
| fault_code             | -        | Current fault |

**Derived (computed on ESP32 + app):**
- Speed (km/h or mph) — needs wheel diameter, pole pairs, gear ratio
- Instant power (W)
- Battery SOC % (voltage-based or from BMS)
- Efficiency (Wh/km or km/kWh)
- Trip distance, average speed, energy used this trip
- Range estimate (simple)

BMS values (when available via CAN): total voltage, SOC, individual cell voltages, balancing state, temps.

## Tech Stack (Proposed)

**Firmware**
- PlatformIO + Arduino framework (easy) **or** ESP-IDF
- LVGL for rich UI on the display (gauges, pages, animations)
- Graphics libs: LovyanGFX or TFT_eSPI / Arduino_GFX
- BLE: NimBLE-Arduino (recommended, lower resource)
- VESC comms: SolidGeek/VescUart (UART) or custom CAN + packet parser

**Mobile App**
- Flutter (strongly recommended for beautiful cross-platform UIs + great BLE/charts)
- Packages: `flutter_blue_plus`, `fl_chart` or `syncfusion_flutter_charts`, `path_provider`, etc.
- Optional: background location, GPX export, ride database (sqlite or drift)

**Other**
- Protocol: Simple binary struct (versioned) over BLE notify for efficiency, or JSON for quick prototyping.
- Config: Stored in ESP32 non-volatile + syncable from app.

## Project Structure

```
MiniBike-Stats/
├── README.md
├── firmware/               # ESP32 code (PlatformIO recommended)
│   ├── platformio.ini
│   └── src/
│       └── main.cpp
├── mobile/                 # Flutter app
│   └── (create with `flutter create minibike_dash`)
├── hardware/               # Schematics, pinouts, BOM, 3D models, enclosure
├── docs/
│   ├── protocol.md         # BLE service/characteristics + packet formats
│   ├── wiring.md
│   ├── display-setup.md
│   └── configuration.md
├── .gitignore
└── LICENSE
```

## Getting Started (High Level)

1. **Choose & wire your hardware**
   - Decide UART or CAN to VESC
   - Connect display + buttons
   - Power safely

2. **Firmware skeleton**
   - Start reading VESC values over UART using a library
   - Compute speed/distance/SOC
   - Drive a basic screen (text + bars first)
   - Advertise a BLE service and notify telemetry every ~200ms

3. **Mobile app**
   - Scan/connect to your ESP32
   - Subscribe to telemetry characteristic
   - Display big speedo + key metrics + live line charts
   - Add trip reset, settings (wheel size etc.)

4. **Polish**
   - Multiple pages on both screen and app
   - Power mode / lights controls
   - Ride logging + history
   - OTA updates for firmware

See `docs/` (to be populated) and examples in `firmware/examples/`.

## Roadmap Ideas

- [ ] Reliable VESC UART + CAN readers
- [ ] LVGL UI with speed gauge, multi-page layout, brightness control
- [ ] BLE telemetry streaming + simple command channel
- [ ] Flutter app: dashboard, trip, charts, settings
- [ ] Persistent odometer + trip stats across reboots
- [ ] BMS integration
- [ ] Ride recording (local on phone + optional cloud)
- [ ] Config sync and calibration (wheel size, battery params)
- [ ] Over-the-air firmware updates
- [ ] GPS speed + maps integration
- [ ] Theming / night mode, units toggle (metric/imperial)

## Contributing

PRs, issues, and hardware variants welcome! Start by opening an issue describing your screen choice, VESC model, and what you want to build.

## License

MIT (or your preference). VESC protocol and libraries retain their own licenses.

## Related / Credits

- VESC project by Benjamin Vedder: https://vesc-project.com/
- VESC Dash 35B & packages: https://github.com/vedderb/vesc_pkg/tree/main/dash35b
- VescUart Arduino lib: https://github.com/SolidGeek/VescUart
- Many DIY ESP32 VESC display projects on Endless-Sphere and esk8 forums

---

**Ready to hack?** Clone/fork, pick your display, and let's build a sweet custom dash for your minibike!

Questions or want help scaffolding specific parts (firmware skeleton, BLE protocol, Flutter starter, wiring diagram ideas)? Open an issue or describe what you have.
