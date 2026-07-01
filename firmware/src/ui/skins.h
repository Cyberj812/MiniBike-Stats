#pragma once

/**
 * MiniBike Stats - UI Skins
 * 
 * Different visual presentations ("skins") for the local display.
 * Each skin complements the telemetry data differently:
 * 
 * - DIGITAL : Modern, precise, information-dense (default)
 * - ANALOG  : Classic gauge / speedo feel
 * - MINIMAL : Big speed + critical status only (great for bright sun)
 * - RACE    : High-contrast, performance focused, warnings prominent
 * 
 * When you add a real display + LVGL, implement the lv_... calls inside
 * each render function. The rest of the firmware stays the same.
 */

#include <Arduino.h>
#include "telemetry.h"   // forward declaration of our Telemetry struct

enum class UiSkin : uint8_t {
  DIGITAL = 0,
  ANALOG,
  MINIMAL,
  RACE,
  COUNT
};

struct SkinInfo {
  const char* name;
  const char* description;
};

// Human readable info (used by app + serial)
constexpr SkinInfo skinInfo[] = {
  {"Digital",   "Clean numbers, progress bars, balanced info"},
  {"Analog",    "Large speed arc + classic gauge aesthetics"},
  {"Minimal",   "Huge speed + tiny icons. Maximum readability"},
  {"Race",      "High contrast. Power/temp emphasis. Red/amber alerts"}
};

inline const char* getSkinName(UiSkin skin) {
  uint8_t idx = (uint8_t)skin;
  if (idx >= (uint8_t)UiSkin::COUNT) return "Unknown";
  return skinInfo[idx].name;
}

// ------------------------------------------------------------------
// SKIN RENDER FUNCTIONS
// ------------------------------------------------------------------

// Current implementation produces nice formatted serial output
// that visually represents each skin. 
// Replace the body with actual LVGL screen drawing when ready.

inline void renderDigitalSkin(const Telemetry& t, bool forceRedraw = false) {
  static uint32_t last = 0;
  if (millis() - last < 300 && !forceRedraw) return;
  last = millis();

  Serial.println("┌──────────────── DIGITAL SKIN ────────────────┐");
  Serial.printf("│ SPEED: %6.1f km/h   SOC: %5.1f%%            │\n", t.speed_kmh, t.soc);
  Serial.printf("│ POWER: %7.0f W     BAT: %5.1f V           │\n", t.power_w, t.v_in);
  Serial.printf("│ TEMP:  MOS %4.1f°C  MOT %4.1f°C            │\n", t.temp_mos, t.temp_motor);
  Serial.printf("│ TRIP:  %7.2f km                           │\n", t.trip_km);
  Serial.println("└─────────────────────────────────────────────┘");
}

inline void renderAnalogSkin(const Telemetry& t, bool forceRedraw = false) {
  static uint32_t last = 0;
  if (millis() - last < 300 && !forceRedraw) return;
  last = millis();

  // Simulate analog speedo look
  int speed = (int)t.speed_kmh;
  Serial.println("┌─────────────── ANALOG GAUGE ────────────────┐");
  Serial.printf("│   Speedo:  %3d km/h     [ARC]               │\n", speed);
  Serial.printf("│   Needle at ~%2d%% of scale                 │\n", (int)(t.speed_kmh / 50.0f * 100));
  Serial.printf("│   Batt Bar: %5.1fV   [%3.0f%%]             │\n", t.v_in, t.soc);
  Serial.println("└─────────────────────────────────────────────┘");
}

inline void renderMinimalSkin(const Telemetry& t, bool forceRedraw = false) {
  static uint32_t last = 0;
  if (millis() - last < 400 && !forceRedraw) return;
  last = millis();

  Serial.println("┌─────────────── MINIMAL ─────────────────────┐");
  Serial.printf("│  %6.1f                                     │\n", t.speed_kmh);
  Serial.printf("│  km/h     %5.0f%%  %5.1fV                 │\n", t.soc, t.v_in);
  Serial.printf("│  %4.0fW  %4.1f°C                          │\n", t.power_w, t.temp_mos);
  Serial.println("└─────────────────────────────────────────────┘");
}

inline void renderRaceSkin(const Telemetry& t, bool forceRedraw = false) {
  static uint32_t last = 0;
  if (millis() - last < 250 && !forceRedraw) return;
  last = millis();

  // High contrast + warning emphasis
  const char* tempStatus = (t.temp_mos > 80 || t.temp_motor > 80) ? "!! HOT !!" : "OK";
  const char* powerStatus = (t.power_w > 4000) ? "HIGH PWR" : "     ";

  Serial.println("┌─────────────── RACE MODE ───────────────────┐");
  Serial.printf("│ SPD %6.1f   SOC %5.1f%%   %s            │\n", t.speed_kmh, t.soc, powerStatus);
  Serial.printf("│ PWR %7.0fW  MOS %5.1f°C %s            │\n", t.power_w, t.temp_mos, tempStatus);
  Serial.printf("│ TRIP %6.2fkm  DUTY %5.1f%%                │\n", t.trip_km, t.duty * 100.0f);
  Serial.println("└─────────────────────────────────────────────┘");
}

// Master dispatcher
inline void renderSkin(UiSkin skin, const Telemetry& t, bool force = false) {
  switch (skin) {
    case UiSkin::DIGITAL: renderDigitalSkin(t, force); break;
    case UiSkin::ANALOG:  renderAnalogSkin(t, force);  break;
    case UiSkin::MINIMAL: renderMinimalSkin(t, force); break;
    case UiSkin::RACE:    renderRaceSkin(t, force);    break;
    default:              renderDigitalSkin(t, force); break;
  }
}
