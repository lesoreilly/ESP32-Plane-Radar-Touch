#pragma once

namespace ui {

/** Draw the static sonar/radar grid (black disc, green overlay, labels). */
void radarDisplayDraw();

/** Redraw aircraft only (blits cached grid; no full-screen clear). */
void radarDisplayRefreshAircraft();

/**
 * Handle a tap at display coordinates.
 * Plane icon, tag, or rim dot opens a detail card over the radar.
 * With the card open, tap it (or empty space) to dismiss; tap another
 * plane to switch. Footer still cycles range when no card is showing.
 */
bool radarDisplayOnTouch(int x, int y);

}  // namespace ui
