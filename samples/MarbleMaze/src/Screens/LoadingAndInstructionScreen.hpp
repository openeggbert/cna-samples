#pragma once

// LoadingAndInstructionScreen.hpp — C++ port of
// Screens/LoadingAndInstructionScreen.cs (XNA 4.0 MarbleMaze sample).
//
// Deviation (see missing.md and GameplayScreen.hpp's own header comment): the C#
// original spins up a background System.Threading.Thread to call
// GameplayScreen.LoadAssets() while this screen shows "Loading...". CNA's EasyGL
// graphics resources (VertexBuffer/IndexBuffer/Texture2D/BasicEffect, all created
// by GameplayScreen::LoadAssets()) are not safe to create from a thread other than
// the one owning the GL context, so this port calls LoadAssets() synchronously
// instead -- the "Loading..." text still gets one Draw() call's worth of visible
// time (isLoading_ is set on tap, and the actual transition happens the *next*
// Update()) even though the load itself completes instantly given this sample's
// small asset set.

#include <memory>
#include <optional>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureType.hpp"
#include "System/TimeSpan.hpp"

#include "../ScreenManager/GameScreen.hpp"
#include "../ScreenManager/ScreenManager.hpp"

namespace MarbleMazeSample {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Input::Touch::GestureType;

class GameplayScreen; // forward declaration -- see file header comment.

class LoadingAndInstructionScreen : public GameScreen {
public:
    LoadingAndInstructionScreen() {
        setTransitionOnTime(System::TimeSpan::FromSeconds(0));
        setTransitionOffTime(System::TimeSpan::FromSeconds(0.5));
        setEnabledGestures(GestureType::Tap);
    }

    // Deferred to Screens/ScreensGlue.hpp -- constructs GameplayScreen.
    void LoadContent() override;
    void HandleInput(InputState& input) override;
    void Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override;

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        SpriteBatch& spriteBatch = GetScreenManager()->getSpriteBatch();

        spriteBatch.Begin();

        spriteBatch.Draw(*background_, Vector2(0, 0), mul(Color::White, TransitionAlpha()));

        if (isLoading_) {
            std::string text = "Loading...";
            Vector2 size = font_->MeasureString(text);
            auto& viewport = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty();
            Vector2 position((viewport.getWidthProperty() - size.X) / 2.0f, (viewport.getHeightProperty() - size.Y) / 2.0f);
            spriteBatch.DrawString(*font_, text, position, Color::White);
        }

        spriteBatch.End();
    }

private:
    std::optional<Texture2D> background_;
    std::optional<SpriteFont> font_;
    bool isLoading_ = false;
    bool loadedThisFrame_ = false;
    std::shared_ptr<GameplayScreen> gameplayScreen_;
};

} // namespace MarbleMazeSample
