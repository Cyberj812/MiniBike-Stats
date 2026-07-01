# Wiring Guide

**WARNING**: High voltage battery systems are dangerous. Double-check polarity and use appropriate fusing, insulation, and PPE. Incorrect wiring can destroy components or cause fire/injury.

## Recommended Connections

### Power
- ESP32 and display powered from a clean 5V rail (or 3.3V logic side)
- Use a good buck converter from your main pack (12-60V+ depending on bike)
- Add capacitors near the ESP32
- Common ground between VESC, ESP32, BMS, display, buttons

### VESC Communication

**UART (easiest to start)**
- VESC TX -> ESP32 RX (choose a hardware UART, e.g. Serial2 on ESP32)
- VESC RX -> ESP32 TX
- GND shared
- Typical baud: 115200 (configurable in VESC Tool)
- Many VESCs have a dedicated "UART" port (often 6-pin or 4-pin)

**CAN (preferred for multi-device)**
- VESC CANH <-> ESP32 CANH (via transceiver like SN65HVD230 or built-in if available)
- VESC CANL <-> ESP32 CANL
- 120Ω termination resistors at both ends of bus (or enable on devices)
- Use twisted pair or proper cable
- All devices must use same baud (default often 500kbps or 1Mbps for VESC)

### Display
Depends on your module:
- SPI (most common): MOSI, MISO, CLK, CS, DC, RST, LED/BL
- Parallel RGB (higher performance)
- I2C for some small OLEDs

Check your specific board's pinout. Many Waveshare/Elecrow boards have example pin maps.

### Buttons / Encoder
- 3-5 physical buttons (or one encoder + button)
- Use internal pull-ups or external
- Debounce in firmware (or LVGL input device driver)

### Optional
- GPS: UART (Neo-M8 or similar)
- SD card: SPI (HSPI)
- Lights relay or MOSFETs: GPIO via logic level shifter or appropriate driver
- Buzzer: GPIO + transistor
- Temperature sensors (extra)

## Example Pin Map (Adjust to your board)

| Function       | ESP32 Pin (example) | Notes |
|----------------|---------------------|-------|
| VESC UART TX   | GPIO 17             | ESP32 RX |
| VESC UART RX   | GPIO 16             | ESP32 TX |
| Display CS     | GPIO 5              | |
| Display DC     | GPIO 4              | |
| Display RST    | GPIO 15             | |
| Display SCK    | GPIO 18             | SPI |
| Display MOSI   | GPIO 23             | |
| Backlight      | GPIO 21             | PWM capable |
| Button 1-4     | GPIO 32,33,34,35    | Input + pullup |
| CAN TX/RX      | (if using TWAI)     | Specific pins for ESP32 TWAI |

Consult your display datasheet + ESP32 technical reference for valid pins (especially for HSPI/VSPI and strapping pins).

## Julet / Waterproof Connectors

The commercial dash uses Julet connectors. Consider using them or Anderson / XT / bullet connectors for your build for weather resistance.

## Testing Order
1. Power ESP32 + display alone (no VESC)
2. Flash basic "hello" + display test
3. Add buttons
4. Connect to VESC via UART, read FW version or basic values first
5. Full telemetry loop
6. Add BLE
7. Add mobile app

Always lift the wheel when first testing throttle-related behavior.
