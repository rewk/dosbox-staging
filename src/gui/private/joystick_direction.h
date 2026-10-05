// SPDX-FileCopyrightText:  2026-2026 The DOSBox Staging Team
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef DOSBOX_GUI_JOYSTICK_DIRECTION_H
#define DOSBOX_GUI_JOYSTICK_DIRECTION_H

#include <cstdint>

// Directions are expressed with the SDL_HAT_* bit values (up = 1, right = 2,
// down = 4, left = 8, diagonals are combinations, 0 is centred), so sticks
// and hats share the same representation.

// Deflection beyond which a stick or axis activates an on/off event (e.g. a
// key press)
constexpr int AxisActivationThreshold = 25000;

// How far past a sector boundary, in degrees, the stick must go to leave the
// current direction
constexpr double StickHysteresisMarginDeg = 7.0;

// The keep zone of a direction must not reach its neighbours' centres, 45
// degrees away
static_assert(StickHysteresisMarginDeg >= 0 && StickHysteresisMarginDeg < 22.5,
              "Stick hysteresis margin must stay below half a sector");

// Converts a stick position (SDL axis values, negative y is up) into one of
// the 8 directions, or centred. Beyond the dead disc (the activation
// threshold), the direction is picked by 45-degree sectors, like a D-pad.
// The previous direction is kept until the stick goes past its sector
// boundary by the hysteresis margin.
uint8_t StickPositionToDirection(int x, int y, uint8_t previous_direction);

#endif
