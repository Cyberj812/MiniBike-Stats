import 'package:flutter/material.dart';
import 'package:provider/provider.dart';

import 'package:minibike_stats/models/telemetry_packet.dart';
import 'package:minibike_stats/services/minibike_ble.dart';
import 'package:minibike_stats/themes/skins.dart';

void main() {
  runApp(
    ChangeNotifierProvider(
      create: (_) => MiniBikeBle(),
      child: const MiniBikeStatsApp(),
    ),
  );
}

class MiniBikeStatsApp extends StatelessWidget {
  const MiniBikeStatsApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'MiniBike Stats',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(
          seedColor: Colors.blue,
          brightness: Brightness.dark,
        ),
        useMaterial3: true,
      ),
      home: const DashboardScreen(),
      debugShowCheckedModeBanner: false,
    );
  }
}

class DashboardScreen extends StatelessWidget {
  const DashboardScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final ble = context.watch<MiniBikeBle>();
    final telemetry = ble.latestTelemetry;

    return Scaffold(
      appBar: AppBar(
        title: const Text('MiniBike Stats'),
        actions: [
          Icon(
            ble.isConnected ? Icons.bluetooth_connected : Icons.bluetooth_disabled,
            color: ble.isConnected ? Colors.green : Colors.red,
          ),
          const SizedBox(width: 16),
        ],
      ),
      body: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          children: [
            // Connection status card
            Card(
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: Row(
                  children: [
                    Expanded(
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Text(
                            ble.statusMessage,
                            style: Theme.of(context).textTheme.titleMedium,
                          ),
                          if (ble.device != null)
                            Text(
                              ble.device!.platformName,
                              style: Theme.of(context).textTheme.bodySmall,
                            ),
                        ],
                      ),
                    ),
                    ElevatedButton.icon(
                      onPressed: ble.isScanning
                          ? null
                          : () async {
                              if (ble.isConnected) {
                                await ble.disconnect();
                              } else {
                                try {
                                  await ble.connect();
                                } catch (e) {
                                  if (context.mounted) {
                                    ScaffoldMessenger.of(context).showSnackBar(
                                      SnackBar(content: Text('Connection failed: $e')),
                                    );
                                  }
                                }
                              }
                            },
                      icon: Icon(ble.isConnected ? Icons.link_off : Icons.link),
                      label: Text(ble.isConnected ? 'Disconnect' : 'Connect'),
                    ),
                  ],
                ),
              ),
            ),

            const SizedBox(height: 16),

            // Main telemetry display
            Expanded(
              child: telemetry == null
                  ? Center(
                      child: Text(
                        ble.isConnected
                            ? 'Waiting for telemetry data...'
                            : 'Connect to your MiniBike to see live stats',
                        textAlign: TextAlign.center,
                        style: Theme.of(context).textTheme.bodyLarge,
                      ),
                    )
                  : _TelemetryDashboard(telemetry: telemetry),
            ),

            const SizedBox(height: 16),

            // Quick action buttons
            if (ble.isConnected)
              Wrap(
                spacing: 12,
                runSpacing: 8,
                children: [
                  ElevatedButton.icon(
                    onPressed: () => ble.cycleDisplaySkin(),
                    icon: const Icon(Icons.palette),
                    label: const Text('Cycle Display Skin'),
                  ),
                  ElevatedButton.icon(
                    onPressed: () => ble.resetTrip(),
                    icon: const Icon(Icons.restart_alt),
                    label: const Text('Reset Trip'),
                  ),
                ],
              ),

            const SizedBox(height: 8),
            Text(
              'Real-time over BLE • 5 Hz updates',
              style: Theme.of(context).textTheme.bodySmall?.copyWith(
                    color: Colors.grey,
                  ),
            ),
          ],
        ),
      ),
    );
  }
}

class _TelemetryDashboard extends StatelessWidget {
  final TelemetryPacket telemetry;

  const _TelemetryDashboard({required this.telemetry});

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);

    return SingleChildScrollView(
      child: Column(
        children: [
          // Big Speed Display
          Card(
            color: theme.colorScheme.primaryContainer,
            child: Padding(
              padding: const EdgeInsets.symmetric(vertical: 24, horizontal: 16),
              child: Column(
                children: [
                  Text(
                    telemetry.speedKmh.toStringAsFixed(1),
                    style: theme.textTheme.displayLarge?.copyWith(
                      fontWeight: FontWeight.bold,
                      color: theme.colorScheme.onPrimaryContainer,
                    ),
                  ),
                  Text(
                    'km/h',
                    style: theme.textTheme.titleLarge?.copyWith(
                      color: theme.colorScheme.onPrimaryContainer,
                    ),
                  ),
                ],
              ),
            ),
          ),

          const SizedBox(height: 12),

          // Key Stats Grid
          GridView.count(
            crossAxisCount: 2,
            shrinkWrap: true,
            physics: const NeverScrollableScrollPhysics(),
            mainAxisSpacing: 8,
            crossAxisSpacing: 8,
            childAspectRatio: 1.6,
            children: [
              _StatCard(
                label: 'Battery',
                value: '${telemetry.soc.toStringAsFixed(0)}%',
                sub: '${telemetry.vIn.toStringAsFixed(1)} V',
                icon: Icons.battery_charging_full,
              ),
              _StatCard(
                label: 'Power',
                value: '${telemetry.powerW.toStringAsFixed(0)} W',
                sub: '${telemetry.iIn.toStringAsFixed(1)} A',
                icon: Icons.flash_on,
              ),
              _StatCard(
                label: 'Motor',
                value: '${telemetry.tempMotor.toStringAsFixed(1)}°C',
                sub: 'Controller ${telemetry.tempMos.toStringAsFixed(1)}°C',
                icon: Icons.thermostat,
              ),
              _StatCard(
                label: 'Trip',
                value: '${telemetry.tripKm.toStringAsFixed(2)} km',
                sub: 'Duty ${(telemetry.duty * 100).toStringAsFixed(0)}%',
                icon: Icons.route,
              ),
            ],
          ),

          const SizedBox(height: 12),

          // Additional info
          Card(
            child: Padding(
              padding: const EdgeInsets.all(12),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.spaceAround,
                children: [
                  _InfoRow(label: 'Motor Current', value: '${telemetry.iMotor.toStringAsFixed(1)} A'),
                  _InfoRow(label: 'Fault', value: telemetry.fault.toString()),
                  _InfoRow(label: 'Version', value: 'v${telemetry.version}'),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }
}

class _StatCard extends StatelessWidget {
  final String label;
  final String value;
  final String sub;
  final IconData icon;

  const _StatCard({
    required this.label,
    required this.value,
    required this.sub,
    required this.icon,
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(12),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(icon, size: 18),
                const SizedBox(width: 6),
                Text(label, style: Theme.of(context).textTheme.labelMedium),
              ],
            ),
            const Spacer(),
            Text(
              value,
              style: Theme.of(context).textTheme.headlineSmall?.copyWith(
                    fontWeight: FontWeight.bold,
                  ),
            ),
            Text(
              sub,
              style: Theme.of(context).textTheme.bodySmall,
            ),
          ],
        ),
      ),
    );
  }
}

class _InfoRow extends StatelessWidget {
  final String label;
  final String value;

  const _InfoRow({required this.label, required this.value});

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        Text(label, style: Theme.of(context).textTheme.bodySmall),
        Text(value, style: const TextStyle(fontWeight: FontWeight.w600)),
      ],
    );
  }
}
