// SPDX-FileCopyrightText:  2026-2026 The DOSBox Staging Team
// SPDX-License-Identifier: GPL-2.0-or-later

#include "gui/private/joystick_direction.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include <SDL3/SDL_joystick.h>

#include "utils/checks.h"

CHECK_NARROWING();

namespace {

struct Spoke {
	uint8_t direction = 0;

	// Unit vector, positive y is down
	double unit_x = 0.0;
	double unit_y = 0.0;
};

constexpr auto Diagonal = std::numbers::sqrt2 / 2;

constexpr Spoke Spokes[] = {
        {       SDL_HAT_UP,       0.0,      -1.0},
        {  SDL_HAT_RIGHTUP,  Diagonal, -Diagonal},
        {    SDL_HAT_RIGHT,       1.0,       0.0},
        {SDL_HAT_RIGHTDOWN,  Diagonal,  Diagonal},
        {     SDL_HAT_DOWN,       0.0,       1.0},
        { SDL_HAT_LEFTDOWN, -Diagonal,  Diagonal},
        {     SDL_HAT_LEFT,      -1.0,       0.0},
        {   SDL_HAT_LEFTUP, -Diagonal, -Diagonal},
};

double distance_along(const Spoke& spoke, const double x, const double y)
{
	return x * spoke.unit_x + y * spoke.unit_y;
}

} // namespace

uint8_t StickPositionToDirection(const int x, const int y,
                                 const uint8_t previous_direction)
{
	const auto pos_x     = static_cast<double>(x);
	const auto pos_y     = static_cast<double>(y);
	const auto magnitude = std::hypot(pos_x, pos_y);
	if (magnitude <= AxisActivationThreshold) {
		return SDL_HAT_CENTERED;
	}

	// Keep the previous direction up to the hysteresis margin past its
	// sector boundary
	constexpr auto keep_angle_rad = (22.5 + StickHysteresisMarginDeg) *
	                                std::numbers::pi / 180;
	const auto min_along_to_keep  = std::cos(keep_angle_rad) * magnitude;
	for (const auto& spoke : Spokes) {
		if (spoke.direction == previous_direction &&
		    distance_along(spoke, pos_x, pos_y) >= min_along_to_keep) {
			return previous_direction;
		}
	}

	// Otherwise the nearest direction; on an exact sector boundary, the
	// first one in the table wins
	const auto along = [&](const Spoke& spoke) {
		return distance_along(spoke, pos_x, pos_y);
	};
	return std::ranges::max_element(Spokes, {}, along)->direction;
}

std::optional<int> DiagonalIndex(const uint8_t direction)
{
	switch (direction) {
	case SDL_HAT_RIGHTUP: return 0;
	case SDL_HAT_RIGHTDOWN: return 1;
	case SDL_HAT_LEFTDOWN: return 2;
	case SDL_HAT_LEFTUP: return 3;
	default: return {};
	}
}

std::string_view DirectionName(const uint8_t direction)
{
	switch (direction) {
	case SDL_HAT_UP: return "up";
	case SDL_HAT_RIGHT: return "right";
	case SDL_HAT_DOWN: return "down";
	case SDL_HAT_LEFT: return "left";
	case SDL_HAT_RIGHTUP: return "up-right";
	case SDL_HAT_RIGHTDOWN: return "down-right";
	case SDL_HAT_LEFTDOWN: return "down-left";
	case SDL_HAT_LEFTUP: return "up-left";
	default: return "centred";
	}
}

ResolvedDirection ResolveDirection(const uint8_t direction,
                                   const bool has_diagonal_binds)
{
	const auto diagonal_index = DiagonalIndex(direction);
	if (diagonal_index && has_diagonal_binds) {
		return {0, diagonal_index};
	}
	return {direction, {}};
}
