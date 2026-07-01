# Display Setup Notes

The goal is a clean, high-visibility UI similar to (or better than) the VESC Dash 35B: big readable speed, battery gauge, power, temps, trip info, status icons.

## Recommended UI Library

**LVGL v9+** is strongly suggested.
- Professional gauges, animations, multiple screens/pages
- Touch support if your display has it
- Many ready-made examples and widgets (arc for speedo, bar, label, chart)

Alternatives for simpler projects:
- TFT_eSPI + manual drawing (fast on SPI)
- LovyanGFX (very fast)

## Popular Boards & Starting Points

1. **Waveshare ESP32-S3 Touch LCD 3.5" / 4.3"**
   - Excellent documentation and LVGL examples
   - Capacitive touch

2. **Elecrow ESP32 5" RGB HMI**
   - Parallel interface → much higher refresh
   - Great for complex UIs

3. **ESP32-2432S028R (Cheap Yellow Display - CYD)**
   - Cheap entry point, community support, but smaller and lower brightness
   - Good for prototyping

4. **Generic 3.5" ILI9341 320x480 + ESP32-S3**
   - Use LovyanGFX or TFT_eSPI

Search for "LVGL <your board>" — there are usually ready PlatformIO / Arduino examples.

## Brightness & Outdoor Use

- Aim for 500-800+ nits if possible
- Auto-brightness with LDR (light dependent resistor) is nice
- Provide manual brightness control in both local buttons and mobile app

## UI Pages (suggested) + Skins

Skins change the visual treatment of these pages. See `docs/ui-skins.md` for details (Digital, Analog, Minimal, Race).

1. **Main Ride**
   - Large speed (analog arc + digital)
   - Battery % bar + voltage
   - Power (W) or current
   - Temps (small bars or numbers)
   - Status icons (lights, cruise, mode, fault)

2. **Trip / Stats**
   - Trip distance, odo
   - Energy used, avg efficiency
   - Time riding, avg speed
   - Reset trip button

3. **BMS / Advanced** (if available)
   - Cell voltages (bars or list)
   - SOC from BMS
   - Balancing indicator

4. **Settings** (or app-controlled)
   - Brightness, units, wheel config (or view only)

Use LVGL "screens" or tabview / tileview for easy switching.

## Input Devices

- Physical buttons → LVGL `lv_indev_drv_t` with `LV_INDEV_TYPE_BUTTON` or encoder
- Touch if present

## Performance Tips

- Use double buffering / PSRAM where available
- Avoid heavy drawing in the main loop; use LVGL timers / tasks
- Update only changed values
- For speedo, consider a sprite needle or arc + label

## Next Steps

1. Get a working LVGL "hello gauge" on your chosen hardware first
2. Then integrate VescUart or CAN reader
3. Pipe the data into LVGL labels/arcs
4. Add BLE on top

Once you pick a specific display model, open an issue or PR with the board name and we can link exact init code.
