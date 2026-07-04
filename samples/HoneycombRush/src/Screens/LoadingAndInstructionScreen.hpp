#pragma once

// LoadingAndInstructionScreen.hpp — C++ port of Screens/LoadingAndInstructionScreen.cs
// (XNA 4.0 HoneycombRush sample). The original loads GameplayScreen's assets
// on a background System.Threading.Thread; this port loads synchronously
// instead (see missing.md), using a one-frame delay flag so the "Loading..."
// text still gets a chance to draw before the (fast, synchronous) load runs.

#include <memory>
#include <optional>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureType.hpp"
#include "System/TimeSpan.hpp"

#include "../Misc/ConfigurationManager.hpp"
#include "../ScreenManager/GameScreen.hpp"
#include "../ScreenManager/ScreenManager.hpp"

namespace HoneycombRush {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Input::Touch::GestureType;

class GameplayScreen; // forward declaration — LoadContent()/Update() are
                       // defined out-of-line in GameplayScreen.hpp, once the
                       // full type is known.

// Shows instructions and (synchronously) loads the gameplay screen's assets
// when tapped. Port of Screens/LoadingAndInstructionScreen.cs.
class LoadingAndInstructionScreen : public GameScreen {
public:
    LoadingAndInstructionScreen() {
        setTransitionOnTime(System::TimeSpan::FromSeconds(0.0));
        setTransitionOffTime(System::TimeSpan::FromSeconds(0.5));

        setEnabledGestures(GestureType::Tap);
    }

    // Defined out-of-line in GameplayScreen.hpp.
    void LoadContent() override;

    void HandleInput(GameTime& gameTime, InputState& input) override {
        if (!isLoading_) {
            if (!input.Gestures.empty() && input.Gestures[0].getGestureTypeProperty() == GestureType::Tap) {
                LoadResources();
            }
        }

        GameScreen::HandleInput(gameTime, input);
    }

    // Defined out-of-line in GameplayScreen.hpp.
    void Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override;

    void Draw(const GameTime&) override {
        SpriteBatch& spriteBatch = GetScreenManager()->getSpriteBatch();

        spriteBatch.Begin();

        // If loading the gameplay screen's resources, show "Loading..." text.
        if (isLoading_) {
            std::string text = "Loading...";
            Vector2 size = font_->MeasureString(text);
            auto& viewport = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty();
            Vector2 position((float)((viewport.getWidthProperty() - size.X) / 2),
                              (float)((viewport.getHeightProperty() - size.Y) / 2));
            spriteBatch.DrawString(*font_, text, position, Color::White);
        }

        spriteBatch.End();
    }

private:
    void LoadResources() { isLoading_ = true; }

    std::optional<SpriteFont> font_;
    bool isLoading_ = false;
    bool loadPending_ = false;
    std::shared_ptr<GameplayScreen> gameplayScreen_;
};

} // namespace HoneycombRush
