import 'dart:async';
import 'dart:io';

import 'package:flutter/foundation.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:minibike_stats/models/telemetry_packet.dart';

/// BLE communication manager for MiniBike Stats
/// 
/// Connects to ESP32 advertising as "MiniBikeStats"
/// Subscribes to real-time telemetry notifications
class MiniBikeBle extends ChangeNotifier {
  static const String deviceName = "MiniBikeStats";

  // Same UUIDs as firmware
  static final Guid serviceUuid = Guid("a1b2c3d4-0000-0000-0000-00000000beef");
  static final Guid telemetryCharUuid = Guid("a1b2c3d4-1001-0000-0000-00000000beef");
  static final Guid commandCharUuid = Guid("a1b2c3d4-1002-0000-0000-00000000beef");

  BluetoothDevice? _device;
  BluetoothCharacteristic? _telemetryChar;
  BluetoothCharacteristic? _commandChar;

  StreamSubscription<List<int>>? _telemetrySubscription;
  StreamSubscription<BluetoothConnectionState>? _connectionSubscription;

  TelemetryPacket? latestTelemetry;
  bool isConnected = false;
  bool isScanning = false;
  String statusMessage = "Disconnected";

  BluetoothDevice? get device => _device;

  /// Scan and connect to the MiniBike
  Future<void> connect() async {
    if (isConnected) return;

    try {
      statusMessage = "Scanning for $deviceName...";
      notifyListeners();

      isScanning = true;
      notifyListeners();

      // Make sure Bluetooth is on
      if (await FlutterBluePlus.isSupported == false) {
        throw Exception("Bluetooth not supported on this device");
      }

      // Start scan
      await FlutterBluePlus.startScan(
        timeout: const Duration(seconds: 8),
        withNames: [deviceName],
      );

      // Listen for scan results
      final completer = Completer<BluetoothDevice?>();

      final sub = FlutterBluePlus.scanResults.listen((results) {
        for (ScanResult r in results) {
          if (r.device.platformName == deviceName) {
            FlutterBluePlus.stopScan();
            completer.complete(r.device);
            return;
          }
        }
      });

      final foundDevice = await completer.future.timeout(
        const Duration(seconds: 10),
        onTimeout: () => null,
      );

      await sub.cancel();

      if (foundDevice == null) {
        throw Exception("Could not find $deviceName. Make sure the ESP32 is powered on and advertising.");
      }

      _device = foundDevice;
      statusMessage = "Connecting to ${_device!.platformName}...";
      notifyListeners();

      // Connect
      await _device!.connect(
        autoConnect: false,
        timeout: const Duration(seconds: 15),
      );

      // Listen for connection state changes
      _connectionSubscription = _device!.connectionState.listen((state) {
        isConnected = state == BluetoothConnectionState.connected;
        if (!isConnected) {
          _cleanupConnection();
        }
        statusMessage = isConnected ? "Connected" : "Disconnected";
        notifyListeners();
      });

      // Discover services
      await _device!.discoverServices();

      final services = _device!.servicesList;
      final service = services.firstWhere(
        (s) => s.uuid == serviceUuid,
        orElse: () => throw Exception("MiniBike service not found"),
      );

      // Find characteristics
      _telemetryChar = service.characteristics.firstWhere(
        (c) => c.uuid == telemetryCharUuid,
      );
      _commandChar = service.characteristics.firstWhere(
        (c) => c.uuid == commandCharUuid,
      );

      // Subscribe to telemetry notifications (real-time updates)
      if (_telemetryChar!.properties.notify) {
        await _telemetryChar!.setNotifyValue(true);

        _telemetrySubscription = _telemetryChar!.onValueReceived.listen((value) {
          try {
            final packet = TelemetryPacket.fromBytes(value);
            latestTelemetry = packet;
            statusMessage = "Receiving data";
            notifyListeners();
          } catch (e) {
            debugPrint("Failed to parse telemetry: $e");
          }
        }, onError: (e) {
          debugPrint("Telemetry stream error: $e");
        });
      }

      // Request a higher MTU for better performance (Android mostly)
      if (Platform.isAndroid) {
        await _device!.requestMtu(185);
      }

      isConnected = true;
      statusMessage = "Connected - receiving real-time stats";
      notifyListeners();

    } catch (e) {
      statusMessage = "Error: $e";
      debugPrint("BLE connect error: $e");
      await disconnect();
      notifyListeners();
      rethrow;
    } finally {
      isScanning = false;
      notifyListeners();
    }
  }

  /// Send a simple command byte to the ESP32
  Future<void> sendCommand(int commandByte) async {
    if (!isConnected || _commandChar == null) {
      throw Exception("Not connected");
    }

    try {
      await _commandChar!.write([commandByte], withoutResponse: true);
      debugPrint("Sent command: 0x${commandByte.toRadixString(16)}");
    } catch (e) {
      debugPrint("Failed to send command: $e");
      rethrow;
    }
  }

  /// Convenience methods for known commands (match firmware)
  Future<void> cycleDisplaySkin() async => sendCommand(0x01);
  Future<void> resetTrip() async => sendCommand(0x02);

  Future<void> disconnect() async {
    await _cleanupConnection();
    if (_device != null) {
      try {
        await _device!.disconnect();
      } catch (_) {}
    }
    _device = null;
    isConnected = false;
    statusMessage = "Disconnected";
    notifyListeners();
  }

  Future<void> _cleanupConnection() async {
    await _telemetrySubscription?.cancel();
    _telemetrySubscription = null;

    await _connectionSubscription?.cancel();
    _connectionSubscription = null;

    _telemetryChar = null;
    _commandChar = null;

    latestTelemetry = null;
  }

  @override
  void dispose() {
    disconnect();
    super.dispose();
  }
}
