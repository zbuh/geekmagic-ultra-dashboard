#pragma once
#include <cmath>
#include <cstdio>
#include <string>

// Generic display helpers shared by the dashboard pages.
//
// format_scaled() and entry_value() exist so unit handling (W -> kW, Wh -> kWh, ...) and the
// "sensor value, or a fixed label if none is wired" fallback only need to be written once and
// are then reused identically by every generic page/entry.
//
// draw_entry_grid() generalizes the old hardcoded 2x2 "page_tiles" layout: icon on top, primary
// value centered below it, secondary value below that, one quadrant per GridEntry. A blank
// (nullptr/empty) icon means the slot is unused and is simply skipped.

inline std::string format_scaled(float value, const std::string &unit) {
  char buf[32];
  if (std::isnan(value))
    return std::string("--");

  // Power/energy-ish units: switch to the k-prefixed unit once the value reaches 4 digits,
  // e.g. 1200 W -> "1.2kW", 3450 Wh -> "3.5kWh".
  if (unit == "W" || unit == "Wh" || unit == "VA" || unit == "var") {
    if (std::fabs(value) >= 1000.0f) {
      snprintf(buf, sizeof(buf), "%.1fk%s", value / 1000.0f, unit.c_str());
    } else {
      snprintf(buf, sizeof(buf), "%.0f%s", value, unit.c_str());
    }
    return std::string(buf);
  }
  if (unit == "kW" || unit == "kWh" || unit == "kVA" || unit == "kvar") {
    snprintf(buf, sizeof(buf), "%.2f%s", value, unit.c_str());
    return std::string(buf);
  }
  if (unit == "%") {
    snprintf(buf, sizeof(buf), "%.0f%%", value);
    return std::string(buf);
  }
  if (unit == "°C" || unit == "°F") {
    snprintf(buf, sizeof(buf), "%.1f%s", value, unit.c_str());
    return std::string(buf);
  }
  if (unit.empty()) {
    snprintf(buf, sizeof(buf), "%.1f", value);
    return std::string(buf);
  }
  // Fallback for any other unit the user types in a substitution (A, V, ppm, bar, ...).
  snprintf(buf, sizeof(buf), "%.1f %s", value, unit.c_str());
  return std::string(buf);
}

// label wins when set (the "or text" case); otherwise falls back to the formatted sensor value,
// or an empty string if the sensor has no value (unwired slot or sensor not yet received).
inline std::string entry_value(const std::string &label, float sensor_value, const std::string &unit) {
  if (!label.empty())
    return label;
  if (std::isnan(sensor_value))
    return std::string("");
  return format_scaled(sensor_value, unit);
}

// Maps an alarm_control_panel state to an index into the alarm_text_* substitutions array -
// shared by the alarm page draw lambda and the state-change automation that records the
// previous state's name, so the actual (translatable) display strings live in one place: the
// yaml substitutions block, not hardcoded in this header.
inline int alarm_state_index(const std::string &state) {
  if (state == "armed_home") return 1;
  if (state == "armed_away") return 2;
  if (state == "armed_night") return 3;
  if (state == "armed_vacation") return 4;
  if (state == "armed_custom_bypass") return 5;
  if (state == "pending") return 6;
  if (state == "arming") return 7;
  if (state == "disarming") return 8;
  if (state == "triggered") return 9;
  return 0; // disarmed (also the fallback for any unrecognized state)
}

struct GridEntry {
  const char *icon;   // Material Symbols glyph, empty/nullptr = unused slot (skipped entirely)
  std::string primary;
  std::string secondary;
};

// icon_font0..3: one font per entry, not a single shared font - each entry's icon substitution
// is embedded in its own font resource (see big_icon_font_p*e* in the yaml) so that two entries
// pasting the *same* Material Symbols glyph don't trip ESPHome's "duplicate glyph" check, which
// only looks within a single font resource.
template<typename T, typename F>
void draw_entry_grid(T &it, F icon_font0, F icon_font1, F icon_font2, F icon_font3, F value_font, F small_font,
                      GridEntry entries[4], Color accent, Color secondary_color, int ox, int oy) {
  it.line(120 + ox, 4 + oy, 120 + ox, 236 + oy, Color(50, 50, 50));
  it.line(4 + ox, 120 + oy, 236 + ox, 120 + oy, Color(50, 50, 50));

  const int cx[4] = {60, 180, 60, 180};
  const int cy[4] = {14, 14, 134, 134};
  F icon_fonts[4] = {icon_font0, icon_font1, icon_font2, icon_font3};

  for (int i = 0; i < 4; i++) {
    if (entries[i].icon == nullptr || entries[i].icon[0] == '\0')
      continue;
    it.print(cx[i] + ox, cy[i] + oy, icon_fonts[i], accent, TextAlign::TOP_CENTER, entries[i].icon);
    if (!entries[i].primary.empty()) {
      it.print(cx[i] + ox, cy[i] + 46 + oy, value_font, accent, TextAlign::TOP_CENTER, entries[i].primary.c_str());
    }
    if (!entries[i].secondary.empty()) {
      it.print(cx[i] + ox, cy[i] + 80 + oy, small_font, secondary_color, TextAlign::TOP_CENTER,
                entries[i].secondary.c_str());
    }
  }
}
