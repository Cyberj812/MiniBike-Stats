# UI Skins for MiniBike Stats

Skins change how telemetry is visually presented on both the local ESP32 display and the mobile app. They are designed to complement the data — making important information stand out while keeping the riding experience pleasant.

## Available Skins

### 1. Digital (Default)
- Clean digital readouts
- Progress bars for SOC, power, temperature
- Good balance of information density
- Recommended for most users

**Visual characteristics**: Modern, precise, lots of small readable numbers + bars.

### 2. Analog
- Large central speed "gauge" (arc + simulated needle)
- Classic motorcycle / scooter dashboard vibe
- Battery and power shown as secondary arcs or bars

**Visual characteristics**: Warm colors, curved elements, nostalgic feel.

### 3. Minimal
- Extremely large speed number (dominant element)
- Only the most critical secondary data (SOC %, voltage or power)
- Excellent outdoors in bright sun

**Visual characteristics**: High contrast, very few elements, maximum readability at a glance.

### 4. Race
- Emphasizes power and temperature
- Color coded warnings (amber/red)
- Shows duty cycle, peak values, or "delta" numbers
- Designed for spirited riding

**Visual characteristics**: High saturation, red/amber/green traffic light logic, performance focused.

## How Skin Switching Works (Current)

- Firmware cycles skins automatically every ~8 seconds in the demo
- BLE command `0x01` forces a cycle (sent from mobile app)
- Long-press on a physical button (TODO) will cycle on the bike
- The mobile app can display its own skin independently or stay in sync

## Implementation Status

**Firmware** (`firmware/src/ui/skins.h`)
- Currently renders styled text to Serial for each skin
- Easy to replace `renderXxxSkin()` functions with real LVGL code
- Skin enum + metadata table ready for the mobile app to read

**Mobile** (`mobile/lib/themes/skins.dart`)
- Dart enum + theme descriptors
- Ready to drive different widgets, charts, and color schemes

## Adding LVGL Skins (When You Choose a Display)

Example structure for a real display:

```cpp
// In each render function:
void renderDigitalSkin(...) {
  lv_obj_t* scr = lv_scr_act();
  // create labels, arcs, bars...
  lv_label_set_text_fmt(speed_label, "%.0f", t.speed_kmh);
  lv_arc_set_value(speed_arc, (int)t.speed_kmh);
  // apply skin-specific colors / fonts
}
```

See:
- `docs/display-setup.md`
- `firmware/platformio.ini` (LVGL + driver libs commented)

## Design Philosophy

Skins should **complement** the telemetry, not fight it:

- Use color temperature mapping (green = good/cool/efficient, red = warning)
- Make the current most important value largest
- Keep critical safety data (faults, high temps, low SOC) visible in every skin
- Allow the rider to choose based on conditions (bright day → Minimal)

Contributions of new skins or improvements to existing ones are very welcome!
