#include "services/radar_rotation.h"

#include <Preferences.h>

namespace services::radar_rotation {

namespace {
constexpr char kPrefsNamespace[] = "radar";
constexpr char kKeyTopHeading[] = "top_bearing";

float s_top_heading = 0.0f;
}  // namespace

void init() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, true);
  s_top_heading = prefs.getFloat(kKeyTopHeading, 0.0f);
  prefs.end();
}

float topHeading() { return s_top_heading; }

void setTopHeading(float heading) {
  while (heading < 0.0f) heading += 360.0f;
  while (heading >= 360.0f) heading -= 360.0f;
  s_top_heading = heading;

  Preferences prefs;
  prefs.begin(kPrefsNamespace, false);
  prefs.putFloat(kKeyTopHeading, s_top_heading);
  prefs.end();
}

void clear() {
  s_top_heading = 0.0f;
  Preferences prefs;
  if (prefs.begin(kPrefsNamespace, false)) {
    prefs.remove(kKeyTopHeading);
    prefs.end();
  }
}

}  // namespace services::radar_rotation