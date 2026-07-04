#pragma once

// LevelOverScreen.hpp — C++ port of Screens/LevelOverScreen.cs (XNA 4.0
// HoneycombRush sample). Like LoadingAndInstructionScreen, the original loads
// the next GameplayScreen's assets on a background thread; this port loads
// synchronously with a one-frame delay flag instead — see missing.md.

#include <memory>
#include <optional>
#include <string>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureType.hpp"

#include "../Misc/ConfigurationManager.hpp"
#include "../ScreenManager/GameScreen.hpp"
#include "../ScreenManager/ScreenManager.hpp"
#include "HighScoreScreen.hpp"

namespace HoneycombRush {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Input::Touch::GestureType;

class GameplayScreen; // forward declaration — LoadContent()/Update()/
                       // StartNewLevelOrExit() are defined out-of-line in
                       // GameplayScreen.hpp, once the full type is known.

// Shows the outcome of a finished level and advances to the next level (or
// the high score screen). Port of Screens/LevelOverScreen.cs. Note: like the
// original, `text_`/`textSize_` are computed but never actually drawn — only
// the footer "tap to continue" text is rendered in Draw() (a leftover from
// the original sample, reproduced faithfully).
class LevelOverScreen : public GameScreen {
public:
    LevelOverScreen(std::string text, std::optional<DifficultyMode> difficultyMode)
        : text_(std::move(text)), difficultyMode_(difficultyMode) {
        setEnabledGestures(GestureType::Tap);
    }

    // Defined out-of-line in GameplayScreen.hpp.
    void LoadContent() override;

    // Defined out-of-line in GameplayScreen.hpp.
    void Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override;

    void HandleInput(GameTime& gameTime, InputState& input) override {
        if (!input.Gestures.empty()) {
            const GestureSample& sample = input.Gestures[0];
            if (sample.getGestureTypeProperty() == GestureType::Tap) {
                StartNewLevelOrExit(input);
                input.Gestures.clear();
            }
        }

        GameScreen::HandleInput(gameTime, input);
    }

    void Draw(const GameTime&) override {
        SpriteBatch& spriteBatch = GetScreenManager()->getSpriteBatch();

        spriteBatch.Begin();

        std::string tapText = difficultyMode_.has_value() ? "Touch to start next level" : "Touch to end game";
        Vector2 size = font16px_->MeasureString(tapText);
        auto& viewport = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty();

        spriteBatch.DrawString(*font16px_, tapText,
                                Vector2((float)viewport.getWidthProperty() / 2.0f - size.X / 2.0f,
                                        (float)viewport.getHeightProperty() - size.Y - 4.0f),
                                Color::Black);

        spriteBatch.End();
    }

private:
    // Defined out-of-line in GameplayScreen.hpp.
    void StartNewLevelOrExit(InputState& input);

    std::optional<SpriteFont> font36px_;
    std::optional<SpriteFont> font16px_;

    std::string text_;
    bool isLoading_ = false;
    bool loadPending_ = false;
    Vector2 textSize_;

    std::optional<DifficultyMode> difficultyMode_;

    std::shared_ptr<GameplayScreen> gameplayScreen_;
};

} // namespace HoneycombRush
