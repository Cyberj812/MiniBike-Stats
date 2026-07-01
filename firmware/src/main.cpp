/**
 * minibike-esp32-dash - Main firmware skeleton
 *
 * Goals:
 *  - Read telemetry from VESC (UART or CAN)
 *  - Drive local display (LVGL or simple)
 *  - Advertise BLE service and stream telemetry
 *  - Handle buttons, compute derived values (speed, trip, SOC)
 *  - Persist odo/trip/config
 */

#include <Arduino.h>
#include <VescUart.h>
#include <NimBLEDevice.h>

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

// === Telemetry cache (what we send over BLE + show on screen) ===
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
  float trip_km = 0;       // resettable
  float odo_km = 0;        // persistent
  float efficiency = 0;

  uint32_t last_update = 0;
};

Telemetry telem;

// === BLE ===
NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pTelemetryChar = nullptr;
bool deviceConnected = false;

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
  pkt.ts_ms     = millis();
  pkt.v_in      = telem.v_in;
  pkt.i_in      = telem.i_in;
  pkt.i_motor   = telem.i_motor;
  pkt.duty      = telem.duty;
  pkt.rpm       = telem.rpm;
  pkt.temp_mos  = telem.temp_mos;
  pkt.temp_motor= telem.temp_motor;
  pkt.fault     = telem.fault;
  pkt.speed_kmh = telem.speed_kmh;
  pkt.power_w   = telem.power_w;
  pkt.soc       = telem.soc;
  pkt.trip_km   = telem.trip_km;
  // power_mode etc. later

  pTelemetryChar->setValue((uint8_t*)&pkt, sizeof(pkt));
  pTelemetryChar->notify();
}

// === Callbacks ===
class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) override {
    deviceConnected = true;
    Serial.println("BLE client connected");
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
    if (value.length() > 0) {
      uint8_t cmd = value[0];
      Serial.printf("Received command: 0x%02X\n", cmd);
      // TODO: handle RESET_TRIP, SET_MODE, LIGHTS, etc.
    }
  }
};

// === Derived calculations ===
float calculateSpeedKmh(float erpm) {
  // erpm = electrical rpm
  float motor_rpm = erpm / (MOTOR_POLE_PAIRS * 2.0f);  // careful: some libs give erpm already
  // Many VESC libs return "rpm" as eRPM. Verify with your setup.
  float wheel_rpm = motor_rpm / GEAR_RATIO;
  float circumference_m = (WHEEL_DIAMETER_MM / 1000.0f) * PI;
  float speed_ms = wheel_rpm * circumference_m / 60.0f;
  return speed_ms * 3.6f;
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

// === Display stub (replace with real LVGL / TFT code) ===
void updateLocalDisplay() {
  // For now just serial print the important stuff
  static uint32_t lastPrint = 0;
  if (millis() - lastPrint > 500) {
    Serial.printf("V: %.1fV  I: %.1fA  Speed: %.1f km/h  SOC: %.0f%%  Tmos: %.1f°C\n",
                  telem.v_in, telem.i_in, telem.speed_kmh, telem.soc, telem.temp_mos);
    lastPrint = millis();
  }

  // === TODO ===
  // lv_label_set_text_fmt(speed_label, "%.0f", telem.speed_kmh);
  // lv_arc_set_value(speed_arc, (int)telem.speed_kmh);
  // update battery bar, temp bars, etc.
}

// === Setup ===
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== minibike-esp32-dash starting ===");

  // VESC UART
  vescSerial.begin(115200, SERIAL_8N1, VESC_RX_PIN, VESC_TX_PIN);
  vesc.setSerialPort(&vescSerial);
  // vesc.setDebugPort(&Serial); // noisy

  // === BLE Init ===
  NimBLEDevice::init("MinibikeDash");
  NimBLEDevice::setMTU(185);

  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  NimBLEService* pService = pServer->createService(SERVICE_UUID);

  pTelemetryChar = pService->createCharacteristic(
      TELEMETRY_CHAR_UUID,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );

  NimBLECharacteristic* pCmdChar = pService->createCharacteristic(
      COMMAND_CHAR_UUID,
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
  );
  pCmdChar->setCallbacks(new CommandCallbacks());

  pService->start();

  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->start();

  Serial.println("BLE advertising started: MinibikeDash");

  // TODO: init display + LVGL here
  // TODO: load persisted odo/trip/config from Preferences / LittleFS

  Serial.println("Setup complete.");
}

// === Loop ===
void loop() {
  updateTelemetryFromVesc();
  updateLocalDisplay();
  sendTelemetryBLE();

  // Light CPU
  delay(50);   // ~20 Hz loop. Adjust as needed.
}
