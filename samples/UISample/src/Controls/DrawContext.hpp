#pragma once

#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

namespace UISample::Controls {

using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::GraphicsDevice;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::Texture2D;

// A collection of rendering data passed into Control::Draw(). Passing this
// into each Draw call lets controls access what they need without
// introducing a dependency on a top-level object like ScreenManager. Port of
// Controls/DrawContext.cs.
struct DrawContext {
    GraphicsDevice* Device = nullptr;
    const GameTime* GameTimeValue = nullptr;
    SpriteBatch* SpriteBatchValue = nullptr;

    // A single-pixel white texture, useful for drawing boxes/lines and as the
    // fallback texture for an ImageControl with no Texture set.
    Texture2D* BlankTexture = nullptr;

    // Positional offset to draw at. This is a simple positional offset rather
    // than a full transform, so this doesn't easily support full hierarchical
    // transforms. A control's Position is already added to this before its
    // own Draw() is called.
    Vector2 DrawOffset;
};

} // namespace UISample::Controls
