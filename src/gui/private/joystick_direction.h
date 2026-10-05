// SPDX-FileCopyrightText:  2026-2026 The DOSBox Staging Team
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef DOSBOX_GUI_JOYSTICK_DIRECTION_H
#define DOSBOX_GUI_JOYSTICK_DIRECTION_H

#include <cstdint>
#include <optional>
#include <string_view>

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

constexpr int NumDiagonals = 4;

// Index (0 to NumDiagonals - 1) of a diagonal direction, in the order
// up-right, down-right, down-left, up-left; empty if not a diagonal
std::optional<int> DiagonalIndex(uint8_t direction);

// Lower-case name of one of the 8 directions (e.g. "up-left"), for bind names
std::string_view DirectionName(uint8_t direction);

struct ResolvedDirection {
	// Straight directions to activate, as SDL_HAT_* bits
	uint8_t straight = 0;

	// Diagonal to activate instead of its two straight directions
	std::optional<int> diagonal_index = {};
};

// A diagonal with dedicated binds activates those binds only (8-way
// exclusive, for games using one key per diagonal); otherwise its two
// straight directions are activated together.
ResolvedDirection ResolveDirection(uint8_t direction, bool has_diagonal_binds);

#endif
