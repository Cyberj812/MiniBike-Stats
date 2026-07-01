import 'dart:typed_data';

/// Matches the C struct TelemetryPacketV1 exactly (little-endian packed)
/// See firmware/src/main.cpp and docs/protocol.md
class TelemetryPacket {
  final int version;
  final int timestampMs;

  // Raw VESC
  final double vIn;            // Battery voltage
  final double iIn;            // Battery current
  final double iMotor;         // Motor current
  final double duty;           // 0.0 - 1.0
  final double rpm;            // eRPM from VESC
  final double tempMos;        // °C
  final double tempMotor;      // °C
  final int fault;

  // Derived (computed on ESP32)
  final double speedKmh;
  final double powerW;
  final double soc;            // 0-100
  final double tripKm;

  // Status
  final int powerMode;
  final int lights;
  final int cruise;

  TelemetryPacket({
    required this.version,
    required this.timestampMs,
    required this.vIn,
    required this.iIn,
    required this.iMotor,
    required this.duty,
    required this.rpm,
    required this.tempMos,
    required this.tempMotor,
    required this.fault,
    required this.speedKmh,
    required this.powerW,
    required this.soc,
    required this.tripKm,
    required this.powerMode,
    required this.lights,
    required this.cruise,
  });

  /// Parse from BLE notification (List<int> / Uint8List)
  factory TelemetryPacket.fromBytes(List<int> bytes) {
    if (bytes.length < 58) {
      throw Exception('Telemetry packet too short: ${bytes.length} bytes');
    }

    final data = ByteData.view(Uint8List.fromList(bytes).buffer);

    int offset = 0;

    final version = data.getUint8(offset); offset += 1;
    final ts = data.getUint32(offset, Endian.little); offset += 4;

    final vIn = data.getFloat32(offset, Endian.little); offset += 4;
    final iIn = data.getFloat32(offset, Endian.little); offset += 4;
    final iMotor = data.getFloat32(offset, Endian.little); offset += 4;
    final duty = data.getFloat32(offset, Endian.little); offset += 4;
    final rpm = data.getFloat32(offset, Endian.little); offset += 4;
    final tempMos = data.getFloat32(offset, Endian.little); offset += 4;
    final tempMotor = data.getFloat32(offset, Endian.little); offset += 4;
    final fault = data.getUint8(offset); offset += 1;

    final speedKmh = data.getFloat32(offset, Endian.little); offset += 4;
    final powerW = data.getFloat32(offset, Endian.little); offset += 4;
    final soc = data.getFloat32(offset, Endian.little); offset += 4;
    final tripKm = data.getFloat32(offset, Endian.little); offset += 4;

    final powerMode = data.getUint8(offset); offset += 1;
    final lights = data.getUint8(offset); offset += 1;
    final cruise = data.getUint8(offset);

    return TelemetryPacket(
      version: version,
      timestampMs: ts,
      vIn: vIn,
      iIn: iIn,
      iMotor: iMotor,
      duty: duty,
      rpm: rpm,
      tempMos: tempMos,
      tempMotor: tempMotor,
      fault: fault,
      speedKmh: speedKmh,
      powerW: powerW,
      soc: soc,
      tripKm: tripKm,
      powerMode: powerMode,
      lights: lights,
      cruise: cruise,
    );
  }

  @override
  String toString() {
    return 'Telemetry v$version | ${speedKmh.toStringAsFixed(1)} km/h | SOC: ${soc.toStringAsFixed(0)}% | ${powerW.toStringAsFixed(0)}W | ${vIn.toStringAsFixed(1)}V';
  }
}
