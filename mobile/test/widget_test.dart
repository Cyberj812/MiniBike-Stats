// Basic smoke test for MiniBike Stats app.

import 'package:flutter_test/flutter_test.dart';

import 'package:minibike_stats/main.dart';

void main() {
  testWidgets('App builds without crashing', (WidgetTester tester) async {
    // Build our app and trigger a frame.
    await tester.pumpWidget(const MiniBikeStatsApp());

    // Verify that the app title is present.
    expect(find.text('MiniBike Stats'), findsOneWidget);
  });
}
