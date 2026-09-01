#pragma once

namespace services::radar_rotation {

void init();

float topHeading();

void setTopHeading(float heading);

/** Restore 0° (north-up) during a factory reset. */
void clear();

}  // namespace services::radar_rotation