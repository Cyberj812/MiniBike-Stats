#pragma once

#include <Arduino.h>

// Shared telemetry struct used by core logic, BLE, skins, etc.
struct Telemetry {
  float v_in = 0;
  float i_in = 0;
  float i_motor = 0;
  float duty = 0;
  float rpm = 0;
  float temp_mos = 0;
  float temp_motor = 0;
  uint8_t fault = 0;

  float speed_kmh = 0;
  float power_w = 0;
  float soc = 0;
  float trip_km = 0;
  float odo_km = 0;
  float efficiency = 0;

  uint32_t last_update = 0;
};
