/**
 * MiniBike Stats - Main firmware
 *
 * Goals:
 *  - Read telemetry from VESC (UART or CAN)
 *  - Drive local display using swappable skins (see ui/skins.h)
 *  - Advertise BLE service and stream telemetry
 *  - Handle buttons, compute derived values (speed, trip, SOC)
 *  - Persist odo/trip/config
 *
 * See docs/stat-configuration-guide.md for proper configuration of all stats.
 */

#include <Arduino.h>
#include <VescUart.h>
#include <NimBLEDevice.h>
#include <string>  // for std::string in callbacks

#include "telemetry.h"
#include "ui/skins.h"

// === CONFIG - move to header or NVS later ===
#define WHEEL_DIAMETER_MM   400.0f   // adjust for your tire
#define MOTOR_POLE_PAIRS    7        // typical hub or mid-drive
#define GEAR_RATIO          1.0f     // 1.0 = direct drive
#define BATTERY_S           13       // e.g. 13s Li-ion
#define BATTERY_MAX_V       54.6f
#define BATTERY_MIN_V       39.0f

// UART to VESC
#define VESC_RX_PIN 16
#define VESC_TX_PIN 17
HardwareSerial vescSerial(1);   // UART1 or 2 depending on board

VescUart vesc;

// Telemetry data (defined in telemetry.h)
Telemetry telem;

// === UI Skin State ===
UiSkin currentSkin = UiSkin::SKIN_DIGITAL;
uint32_t lastSkinSwitch = 0;

// === BLE ===
NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pTelemetryChar = nullptr;
NimBLECharacteristic* pCmdChar = nullptr;
bool deviceConnected = false;

// Real-time telemetry rate
const uint32_t TELEMETRY_INTERVAL_MS = 200;   // ~5 Hz - good balance for BLE + battery
uint32_t lastTelemetrySend = 0;

#define SERVICE_UUID        "a1b2c3d4-0000-0000-0000-00000000beef"
#define TELEMETRY_CHAR_UUID "a1b2c3d4-1001-0000-0000-00000000beef"
#define COMMAND_CHAR_UUID   "a1b2c3d4-1002-0000-0000-00000000beef"

// Very simple packed telemetry (v1)
struct __attribute__((packed)) TelemetryPacketV1 {
  uint8_t  version = 1;
  uint32_t ts_ms;
  float    v_in;
  float    i_in;
  float    i_motor;
  float    duty;
  float    rpm;
  float    temp_mos;
  float    temp_motor;
  uint8_t  fault;
  float    speed_kmh;
  float    power_w;
  float    soc;
  float    trip_km;
  uint8_t  power_mode;   // stub
  uint8_t  lights;       // stub
  uint8_t  cruise;       // stub
};

void sendTelemetryBLE() {
  if (!deviceConnected || !pTelemetryChar) return;

  TelemetryPacketV1 pkt{};
  pkt.ts_ms      = millis();
  pkt.v_in       = telem.v_in;
  pkt.i_in       = telem.i_in;
  pkt.i_motor    = telem.i_motor;
  pkt.duty       = telem.duty;
  pkt.rpm        = telem.rpm;
  pkt.temp_mos   = telem.temp_mos;
  pkt.temp_motor = telem.temp_motor;
  pkt.fault      = telem.fault;
  pkt.speed_kmh  = telem.speed_kmh;
  pkt.power_w    = telem.power_w;
  pkt.soc        = telem.soc;
  pkt.trip_km    = telem.trip_km;
  pkt.power_mode = 0; // TODO: wire from actual state
  pkt.lights     = 0;
  pkt.cruise     = 0;

  pTelemetryChar->setValue((uint8_t*)&pkt, sizeof(pkt));
  pTelemetryChar->notify();
}

// === Callbacks ===
class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) override {
    deviceConnected = true;
    Serial.println("BLE client connected");

    // MTU is set globally in setup() for better telemetry packet size.
    // NimBLE handles negotiation automatically in most cases.
  }
  void onDisconnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) override {
    deviceConnected = false;
    Serial.println("BLE client disconnected");
    NimBLEDevice::startAdvertising();
  }
};

class CommandCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc) override {
    std::string value = pCharacteristic->getValue();
    if (value.length() == 0) return;

    uint8_t cmd = value[0];
    Serial.printf("Received command: 0x%02X\n", cmd);

    switch (cmd) {
      case 0x01: // CYCLE_SKIN
        {
          uint8_t next = ((uint8_t)currentSkin + 1) % (uint8_t)UiSkin::COUNT;
          currentSkin = (UiSkin)next;
          lastSkinSwitch = millis();
          Serial.printf("[CMD] Skin -> %s\n", getSkinName(currentSkin));
          renderSkin(currentSkin, telem, true);
        }
        break;
      case 0x02: // RESET_TRIP
        telem.trip_km = 0;
        Serial.println("[CMD] Trip reset");
        break;
      // TODO: 0x03 = SET_POWER_MODE, 0x04 = LIGHTS, etc.
      default:
        Serial.println("[CMD] Unknown command");
        break;
    }
  }
};

// === Derived calculations ===
// IMPORTANT: See docs/stat-configuration-guide.md for full explanation + calibration
float calculateSpeedKmh(float vescRpm) {
  // VescUart typically returns electrical RPM (eRPM)
  // motor mechanical RPM = eRPM / (pole_pairs * 2)  in many cases.
  // Test and adjust! Some setups treat vescRpm directly.
  float motorRpm = vescRpm / (MOTOR_POLE_PAIRS * 2.0f);
  float wheelRpm = motorRpm / GEAR_RATIO;
  float circumferenceM = (WHEEL_DIAMETER_MM / 1000.0f) * PI;
  float speedMs = wheelRpm * circumferenceM / 60.0f;
  return speedMs * 3.6f;
}

float calculateSoc(float voltage) {
  if (voltage <= BATTERY_MIN_V) return 0;
  if (voltage >= BATTERY_MAX_V) return 100;
  return (voltage - BATTERY_MIN_V) / (BATTERY_MAX_V - BATTERY_MIN_V) * 100.0f;
}

void updateTelemetryFromVesc() {
  if (vesc.getVescValues()) {
    telem.v_in       = vesc.data.inpVoltage;
    telem.i_in       = vesc.data.avgInputCurrent;
    telem.i_motor    = vesc.data.avgMotorCurrent;
    telem.duty       = vesc.data.dutyCycleNow;
    telem.rpm        = vesc.data.rpm;
    telem.temp_mos   = vesc.data.tempMosfet;
    telem.temp_motor = vesc.data.tempMotor;
    telem.fault      = vesc.data.error;

    telem.speed_kmh  = calculateSpeedKmh(telem.rpm);
    telem.power_w    = telem.v_in * telem.i_in;
    telem.soc        = calculateSoc(telem.v_in);

    // TODO: accumulate trip using tachometer delta + wheel calc
    // telem.trip_km = ...
    // telem.odo_km = ...

    telem.last_update = millis();
  }
}

// === Display / Skin dispatcher ===
// Currently renders to Serial using different "skins".
// When adding LVGL + real display, implement the actual drawing inside skins.h
void updateLocalDisplay() {
  // Switch skin on a long button press simulation (every 8 seconds for demo)
  if (millis() - lastSkinSwitch > 8000) {
    uint8_t next = ((uint8_t)currentSkin + 1) % (uint8_t)UiSkin::COUNT;
    currentSkin = (UiSkin)next;
    lastSkinSwitch = millis();
    Serial.printf("[SKIN] Switched to: %s\n", getSkinName(currentSkin));
  }

  renderSkin(currentSkin, telem);
}

// === Setup ===
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== MiniBike Stats starting ===");

  // VESC UART
  vescSerial.begin(115200, SERIAL_8N1, VESC_RX_PIN, VESC_TX_PIN);
  vesc.setSerialPort(&vescSerial);
  // vesc.setDebugPort(&Serial); // noisy

  // === BLE Init ===
  NimBLEDevice::init("MiniBikeStats");
  NimBLEDevice::setMTU(185);

  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  NimBLEService* pService = pServer->createService(SERVICE_UUID);

  pTelemetryChar = pService->createCharacteristic(
      TELEMETRY_CHAR_UUID,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );

  pCmdChar = pService->createCharacteristic(
      COMMAND_CHAR_UUID,
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
  );
  pCmdChar->setCallbacks(new CommandCallbacks());

  pService->start();

  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->start();

  Serial.println("BLE advertising started: MiniBikeStats");

  // TODO: init display + LVGL here (see ui/skins.h)
  // TODO: load persisted odo/trip/config + currentSkin from Preferences
  // TODO: register button input to cycle skins (long press, double click, etc.)

  Serial.printf("Active skin: %s\n", getSkinName(currentSkin));
  Serial.println("Setup complete.");
}

// === Loop ===
void loop() {
  updateTelemetryFromVesc();
  updateLocalDisplay();

  // Send telemetry at controlled rate for smooth real-time experience
  uint32_t now = millis();
  if (now - lastTelemetrySend >= TELEMETRY_INTERVAL_MS) {
    sendTelemetryBLE();
    lastTelemetrySend = now;
  }

  // Small delay keeps CPU usage reasonable
  delay(20);
}
