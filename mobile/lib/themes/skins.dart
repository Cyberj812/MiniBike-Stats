// MiniBike Stats - Mobile App Skins / Themes
//
// These complement the on-bike display skins.
// The phone can show richer visualizations because of the bigger screen.

enum AppSkin {
  digital,   // Clean modern cards + big numbers
  analog,    // Gauge-heavy, speedo + arc widgets
  minimal,   // Very large speed, subtle secondary data
  race,      // Red/amber aggressive look, prominent power & temp warnings
}

class SkinTheme {
  final String name;
  final String description;
  final int primaryColor;   // 0xAARRGGBB style or use Color

  const SkinTheme({
    required this.name,
    required this.description,
    required this.primaryColor,
  });
}

const Map<AppSkin, SkinTheme> appSkins = {
  AppSkin.digital: SkinTheme(
    name: 'Digital',
    description: 'Balanced, readable, information rich. Good everyday skin.',
    primaryColor: 0xFF00BCD4,
  ),
  AppSkin.analog: SkinTheme(
    name: 'Analog',
    description: 'Classic motorcycle gauge aesthetic with arcs and needles.',
    primaryColor: 0xFFFF9800,
  ),
  AppSkin.minimal: SkinTheme(
    name: 'Minimal',
    description: 'Maximum focus on speed. Everything else is tiny.',
    primaryColor: 0xFFFFFFFF,
  ),
  AppSkin.race: SkinTheme(
    name: 'Race',
    description: 'High contrast. Red/amber for warnings. Performance oriented.',
    primaryColor: 0xFFF44336,
  ),
};

// Example usage ideas:
// - Different fl_chart styles per skin
// - Gauge widgets (syncfusion or custom painter) for Analog skin
// - Color temperature mapping: green (<60°C) → yellow → red (>85°C)
