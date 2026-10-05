// SPDX-FileCopyrightText:  2026-2026 The DOSBox Staging Team
// SPDX-License-Identifier: GPL-2.0-or-later

#include "gui/private/joystick_direction.h"

#include <gtest/gtest.h>

#include <cmath>
#include <numbers>

#include <SDL3/SDL_joystick.h>

constexpr uint8_t Centred   = SDL_HAT_CENTERED;
constexpr uint8_t Up        = SDL_HAT_UP;
constexpr uint8_t Right     = SDL_HAT_RIGHT;
constexpr uint8_t Down      = SDL_HAT_DOWN;
constexpr uint8_t Left      = SDL_HAT_LEFT;
constexpr uint8_t RightUp   = SDL_HAT_RIGHTUP;
constexpr uint8_t RightDown = SDL_HAT_RIGHTDOWN;
constexpr uint8_t LeftUp    = SDL_HAT_LEFTUP;
constexpr uint8_t LeftDown  = SDL_HAT_LEFTDOWN;

// Full deflection of a round-gate stick at 45 degrees: 32767 * cos(45°)
static const int full_diagonal = static_cast<int>(32767 * std::numbers::sqrt2 / 2);

// The 8 directions every 45 degrees, counter-clockwise from right (towards
// up, i.e. negative y)
constexpr uint8_t DirectionAtAngle[] =
        {Right, RightUp, Up, LeftUp, Left, LeftDown, Down, RightDown};

constexpr int NumDirections = 8;

constexpr double SectorDeg     = 45.0;
constexpr double HalfSectorDeg = SectorDeg / 2;
constexpr double KeepDeg       = HalfSectorDeg + StickHysteresisMarginDeg;

constexpr double Magnitude = 30000.0;

static uint8_t direction_at_index(const int index)
{
	return DirectionAtAngle[((index % NumDirections) + NumDirections) % NumDirections];
}

struct Position {
	int x = 0;
	int y = 0;
};

// Position at `angle_deg` counter-clockwise from right, towards up
static Position at_angle(const double angle_deg, const double magnitude = Magnitude)
{
	const auto angle = angle_deg * std::numbers::pi / 180;
	return {static_cast<int>(std::lround(magnitude * std::cos(angle))),
	        static_cast<int>(std::lround(-magnitude * std::sin(angle)))};
}

static uint8_t direction_at(const Position pos, const uint8_t previous = Centred)
{
	return StickPositionToDirection(pos.x, pos.y, previous);
}

TEST(JoystickDirection, CentredStickIsCentred)
{
	EXPECT_EQ(direction_at({0, 0}), Centred);
}

TEST(JoystickDirection, StraightBelowThresholdIsCentred)
{
	EXPECT_EQ(direction_at({24000, 0}), Centred);
	EXPECT_EQ(direction_at({0, -24000}), Centred);
}

TEST(JoystickDirection, MagnitudeEqualToThresholdIsCentred)
{
	EXPECT_EQ(direction_at({AxisActivationThreshold, 0}), Centred);
}

TEST(JoystickDirection, DiagonalWithMagnitudeBelowThresholdIsCentred)
{
	// hypot(17000, 17000) ~= 24042
	EXPECT_EQ(direction_at({17000, -17000}), Centred);
}

TEST(JoystickDirection, InsideDeadDiscPreviousIsDropped)
{
	EXPECT_EQ(direction_at({20000, 0}, Right), Centred);
	EXPECT_EQ(direction_at({0, 0}, Up), Centred);
}

TEST(JoystickDirection, FullStraightDeflections)
{
	EXPECT_EQ(direction_at({32767, 0}), Right);
	EXPECT_EQ(direction_at({-32768, 0}), Left);
	EXPECT_EQ(direction_at({0, 32767}), Down);
	EXPECT_EQ(direction_at({0, -32768}), Up);
}

TEST(JoystickDirection, RoundGateFullDiagonalsBelowPerAxisThreshold)
{
	// The reported bug: each axis stays below the per-axis threshold
	ASSERT_LT(full_diagonal, AxisActivationThreshold);

	EXPECT_EQ(direction_at({full_diagonal, -full_diagonal}), RightUp);
	EXPECT_EQ(direction_at({full_diagonal, full_diagonal}), RightDown);
	EXPECT_EQ(direction_at({-full_diagonal, -full_diagonal}), LeftUp);
	EXPECT_EQ(direction_at({-full_diagonal, full_diagonal}), LeftDown);
}

TEST(JoystickDirection, NegativeExtremesAreLeftUp)
{
	EXPECT_EQ(direction_at({-32768, -32768}), LeftUp);
}

TEST(JoystickDirection, SectorCentresFromCentred)
{
	for (auto i = 0; i < NumDirections; ++i) {
		EXPECT_EQ(direction_at(at_angle(i * SectorDeg)),
		          direction_at_index(i));
	}
}

TEST(JoystickDirection, SectorBoundariesFromCentred)
{
	for (auto i = 0; i < NumDirections; ++i) {
		const auto boundary_deg = i * SectorDeg + HalfSectorDeg;
		EXPECT_EQ(direction_at(at_angle(boundary_deg - 1)),
		          direction_at_index(i));
		EXPECT_EQ(direction_at(at_angle(boundary_deg + 1)),
		          direction_at_index(i + 1));
	}
}

TEST(JoystickDirection, FullRimOnSectorBoundaryFromCentredIsADirection)
{
	// A push from the centre anywhere on the rim must press something
	for (auto i = 0; i < NumDirections; ++i) {
		const auto boundary_deg = i * SectorDeg + HalfSectorDeg;
		const auto direction = direction_at(at_angle(boundary_deg, 32767));
		EXPECT_TRUE(direction == direction_at_index(i) ||
		            direction == direction_at_index(i + 1));
	}
}

TEST(JoystickDirection, PreviousKeptWithinHysteresisMargin)
{
	for (auto i = 0; i < NumDirections; ++i) {
		const auto previous = direction_at_index(i);
		for (const auto side : {-1, 1}) {
			const auto angle_deg = i * SectorDeg + side * (KeepDeg - 1);
			EXPECT_EQ(direction_at(at_angle(angle_deg), previous),
			          previous);
		}
	}
}

TEST(JoystickDirection, PreviousLeftBeyondHysteresisMargin)
{
	for (auto i = 0; i < NumDirections; ++i) {
		const auto previous = direction_at_index(i);
		for (const auto side : {-1, 1}) {
			const auto angle_deg = i * SectorDeg + side * (KeepDeg + 1);
			EXPECT_EQ(direction_at(at_angle(angle_deg), previous),
			          direction_at_index(i + side));
		}
	}
}

TEST(JoystickDirection, NonAdjacentPreviousIsNeverKept)
{
	for (auto i = 0; i < NumDirections; ++i) {
		// Even at the edge of the sector next to the previous direction
		const auto angle_deg = i * SectorDeg + HalfSectorDeg - 1;

		// Every direction but this sector's and the next one, the
		// opposite included
		for (auto j = 2; j < NumDirections; ++j) {
			const auto previous = direction_at_index(i + j);
			EXPECT_EQ(direction_at(at_angle(angle_deg), previous),
			          direction_at_index(i));
		}
	}
}

TEST(JoystickDirection, InvalidPreviousGivesNearest)
{
	EXPECT_EQ(direction_at(at_angle(0), Up | Down), Right);
	EXPECT_EQ(direction_at(at_angle(90), 0xff), Up);
}

TEST(JoystickDirection, DiagonalIndexInClockwiseOrderFromUpRight)
{
	EXPECT_EQ(DiagonalIndex(RightUp), 0);
	EXPECT_EQ(DiagonalIndex(RightDown), 1);
	EXPECT_EQ(DiagonalIndex(LeftDown), 2);
	EXPECT_EQ(DiagonalIndex(LeftUp), 3);
}

TEST(JoystickDirection, DiagonalIndicesFitNumDiagonals)
{
	for (const auto diagonal : {RightUp, RightDown, LeftDown, LeftUp}) {
		const auto index = DiagonalIndex(diagonal);
		ASSERT_TRUE(index.has_value());
		EXPECT_GE(*index, 0);
		EXPECT_LT(*index, NumDiagonals);
	}
}

TEST(JoystickDirection, NoDiagonalIndexForCentredStraightOrInvalid)
{
	for (const auto direction : {Centred, Up, Right, Down, Left}) {
		EXPECT_FALSE(DiagonalIndex(direction).has_value());
	}
	// Opposite directions together are not a diagonal
	EXPECT_FALSE(DiagonalIndex(Up | Down).has_value());
	EXPECT_FALSE(DiagonalIndex(Left | Right).has_value());
	EXPECT_FALSE(DiagonalIndex(0xff).has_value());
}

TEST(JoystickDirection, DirectionNames)
{
	EXPECT_EQ(DirectionName(Up), "up");
	EXPECT_EQ(DirectionName(Right), "right");
	EXPECT_EQ(DirectionName(Down), "down");
	EXPECT_EQ(DirectionName(Left), "left");
	EXPECT_EQ(DirectionName(RightUp), "up-right");
	EXPECT_EQ(DirectionName(RightDown), "down-right");
	EXPECT_EQ(DirectionName(LeftDown), "down-left");
	EXPECT_EQ(DirectionName(LeftUp), "up-left");
}

TEST(JoystickDirection, DirectionNameOfNonDirectionIsHarmless)
{
	EXPECT_EQ(DirectionName(Centred), "centred");
	EXPECT_FALSE(DirectionName(Up | Down).empty());
	EXPECT_FALSE(DirectionName(0xff).empty());
}

TEST(JoystickDirection, ResolveStraightPassesThroughRegardlessOfDiagonalBinds)
{
	for (const auto direction : {Up, Right, Down, Left}) {
		for (const auto has_diagonal_binds : {false, true}) {
			const auto resolved = ResolveDirection(direction,
			                                       has_diagonal_binds);
			EXPECT_EQ(resolved.straight, direction);
			EXPECT_FALSE(resolved.diagonal_index.has_value());
		}
	}
}

TEST(JoystickDirection, ResolveDiagonalWithoutBindsActivatesBothStraights)
{
	for (const auto diagonal : {RightUp, RightDown, LeftDown, LeftUp}) {
		const auto resolved = ResolveDirection(diagonal, false);
		EXPECT_EQ(resolved.straight, diagonal);
		EXPECT_FALSE(resolved.diagonal_index.has_value());
	}
}

TEST(JoystickDirection, ResolveDiagonalWithBindsActivatesDiagonalOnly)
{
	for (const auto diagonal : {RightUp, RightDown, LeftDown, LeftUp}) {
		const auto resolved = ResolveDirection(diagonal, true);
		EXPECT_EQ(resolved.straight, Centred);
		EXPECT_EQ(resolved.diagonal_index, DiagonalIndex(diagonal));
	}
}

TEST(JoystickDirection, ResolveCentredActivatesNothing)
{
	for (const auto has_diagonal_binds : {false, true}) {
		const auto resolved = ResolveDirection(Centred, has_diagonal_binds);
		EXPECT_EQ(resolved.straight, Centred);
		EXPECT_FALSE(resolved.diagonal_index.has_value());
	}
}
