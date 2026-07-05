#pragma once

// BlackjackCommon.hpp -- common XNA type aliases for the `Blackjack`
// namespace. C++ `using` declarations are scoped to the namespace block they
// appear in, so aliases declared inside `namespace CardsFramework { ... }`
// (e.g. in AnimatedGameComponent.hpp) are not visible from `namespace
// Blackjack { ... }` even though Blackjack classes derive from CardsFramework
// ones. Every Blackjack/*.hpp file includes this first instead of repeating
// the same using-block; re-including/re-declaring the same alias elsewhere is
// harmless (not an error).

#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/IGameComponent.hpp"
#include "Microsoft/Xna/Framework/PlayerIndex.hpp"
#include "Microsoft/Xna/Framework/Point.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "System/TimeSpan.hpp"
#include "System/DateTime.hpp"

namespace Blackjack {

using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::IGameComponent;
using Microsoft::Xna::Framework::PlayerIndex;
using Microsoft::Xna::Framework::Point;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using System::TimeSpan;
using System::DateTime;

} // namespace Blackjack
