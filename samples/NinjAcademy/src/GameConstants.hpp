#pragma once

// GameConstants.hpp — C++ port of GameConstants.cs (XNA 4.0 NinjAcademy
// sample). Constants used for positioning UI elements and defining the
// game's behavior.

#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "System/TimeSpan.hpp"

namespace NinjAcademy {

using Microsoft::Xna::Framework::Vector2;

namespace GameConstants {

// Menu screen constants
inline constexpr int MainMenuLeft = 400;
inline constexpr int MainMenuTop = 265;
inline constexpr int MainMenuEntryGap = 70;

// Pause screen constants
inline constexpr int PauseMenuLeft = 420;
inline constexpr int PauseMenuTop = 320;
inline constexpr int PauseMenuEntryGap = 70;

// HUD constants
inline const Vector2 HitPointsOrigin(740.0f, 15.0f);
inline constexpr float HitPointsSpace = 5.0f;
inline const Vector2 ScorePosition(10.0f, 15.0f);

// Launch constants
inline const Vector2 LaunchAcceleration(0.0f, 500.0f);
inline constexpr float OffScreenYCoordinate = 500.0f;

// Sword slash constants
inline const System::TimeSpan SwordSlashFadeDuration = System::TimeSpan::FromMilliseconds(150);

// Target constants
inline const Vector2 UpperTargetAreaTopLeft(208.0f, 169.0f);
inline const Vector2 MiddleTargetAreaTopLeft(208.0f, 247.0f);
inline const Vector2 LowerTargetAreaTopLeft(208.0f, 320.0f);

inline const Vector2 UpperTargetAreaBottomRight(591.0f, 226.0f);
inline const Vector2 MiddleTargetAreaBottomRight(591.0f, 298.0f);
inline const Vector2 LowerTargetAreaBottomRight(591.0f, 370.0f);

inline const Vector2 UpperTargetOrigin(750.0f, 200.0f);
inline const Vector2 UpperTargetDestination(50.0f, 200.0f);
inline const Vector2 MiddleTargetOrigin(50.0f, 273.0f);
inline const Vector2 MiddleTargetDestination(750.0f, 273.0f);
inline const Vector2 LowerTargetOrigin(750.0f, 344.0f);
inline const Vector2 LowerTargetDestination(50.0f, 344.0f);
inline constexpr float TargetSpeed = 75.0f;

inline constexpr float TargetRadius = 28.0f;

// Drawing order constants
inline constexpr int HUDDrawOrder = 50;
inline constexpr int RoomDrawOrder = 20;
inline constexpr int DefaultDrawOrder = 30;
inline constexpr int TargetDrawOrder = 10;
inline constexpr int FallingTargetDrawOrder = 5;
inline constexpr int ThrowingStarsDrawOrder = 25;
inline constexpr int SwordSlashDrawOrder = 40;

// Throwing star constants
inline const Vector2 ThrowingStarOrigin(400.0f, 510.0f);
inline const System::TimeSpan ThrowingStarFlightDuration = System::TimeSpan::FromMilliseconds(250);
inline constexpr float ThrowingStarEndScale = 0.25f;

// High-score screen constants
inline constexpr int HighScorePlaceLeftMargin = 70;
inline constexpr int HighScoreNameLeftMargin = 120;
inline constexpr int HighScoreScoreLeftMargin = 575;
inline constexpr int HighScoreTitleTopMargin = 25;
inline constexpr int HighScoreTopMargin = 100;
inline constexpr int HighScoreVerticalJump = 50;

} // namespace GameConstants

} // namespace NinjAcademy
