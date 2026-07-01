// Basic smoke test for MiniBike Stats app.

import 'package:flutter_test/flutter_test.dart';
import 'package:provider/provider.dart';

import 'package:minibike_stats/main.dart';
import 'package:minibike_stats/services/minibike_ble.dart';

void main() {
  testWidgets('App builds without crashing', (WidgetTester tester) async {
    // Build our app wrapped with the required Provider and trigger a frame.
    await tester.pumpWidget(
      ChangeNotifierProvider<MiniBikeBle>(
        create: (_) => MiniBikeBle(),
        child: const MiniBikeStatsApp(),
      ),
    );

    // Verify that the app title is present.
    expect(find.text('MiniBike Stats'), findsOneWidget);
  });
}
