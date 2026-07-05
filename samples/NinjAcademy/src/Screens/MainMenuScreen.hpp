#pragma once

// MainMenuScreen.hpp — C++ port of Screens/MainMenuScreen.cs (XNA 4.0
// NinjAcademy sample). Windows Phone tombstoning (the "resume saved game?"
// message box, PhoneApplicationService state) is dropped, matching this
// project's established precedent -- see missing.md. This means
// StartSelected() always takes the "no saved game" branch.

#include <cmath>
#include <memory>

#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include "../AudioManager.hpp"
#include "../ScreenManager/MenuScreen.hpp"
#include "BackgroundScreen.hpp"

namespace NinjAcademy {

using Microsoft::Xna::Framework::Graphics::Texture2D;

class LoadingScreen;  // forward declaration -- LoadContent/Update/StartSelected
class HighScoreScreen; // defined out-of-line in GameplayScreen.hpp.

// Port of Screens/MainMenuScreen.cs.
class MainMenuScreen : public MenuScreen {
public:
    MainMenuScreen() : MenuScreen("") {
        auto startGameMenuEntry = std::make_shared<MenuEntry>("Start");
        auto highScoreMenuEntry = std::make_shared<MenuEntry>("High Score");
        auto exitMenuEntry = std::make_shared<MenuEntry>("Exit");

        startGameMenuEntry->Selected = [this](PlayerIndex p) { StartSelected(p); };
        highScoreMenuEntry->Selected = [this](PlayerIndex p) { HighScoreSelected(p); };
        exitMenuEntry->Selected = [this](PlayerIndex p) { OnCancel(p); };

        MenuEntries().push_back(startGameMenuEntry);
        MenuEntries().push_back(highScoreMenuEntry);
        MenuEntries().push_back(exitMenuEntry);
    }

    // Defined out-of-line in GameplayScreen.hpp (needs HighScoreScreen/AudioManager).
    void LoadContent() override;

    void Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override;

    void HandleInput(InputState& input) override {
        MenuScreen::HandleInput(input);
        if (isExiting_) return;
    }

    void Draw(const GameTime& gameTime) override {
        MenuScreen::Draw(gameTime);

        SpriteBatch& spriteBatch = GetScreenManager()->getSpriteBatch();

        spriteBatch.Begin();
        spriteBatch.Draw(ninjaTexture_, ninjaPosition_ + ninjaOffset_, Color::White);
        spriteBatch.Draw(titleTexture_, titlePosition_ + titleOffset_, Color::White);
        spriteBatch.End();
    }

protected:
    // Arranges all menu entries at the lower-right side of the screen.
    void UpdateMenuEntryLocations() override {
        int menuEntryVerticalPlacement = GameConstants::MainMenuTop;

        for (auto& entry : MenuEntries()) {
            float entryWidth = GetScreenManager()->getFont().MeasureString(entry->Text()).X;
            int menuEntryHorizontalPlacement =
                (int)(GameConstants::MainMenuLeft + maxEntryWidth_ / 2 - entryWidth / 2);
            entry->setPosition(Vector2((float)menuEntryHorizontalPlacement, (float)menuEntryVerticalPlacement));
            menuEntryVerticalPlacement += GameConstants::MainMenuEntryGap;
        }
    }

    void OnCancel(PlayerIndex playerIndex) override {
        (void)playerIndex;
        isExiting_ = true;
        AudioManager::StopMusic();
    }

private:
    // Defined out-of-line in GameplayScreen.hpp (needs LoadingScreen).
    void StartSelected(PlayerIndex playerIndex);

    // Defined out-of-line in GameplayScreen.hpp (needs HighScoreScreen).
    void HighScoreSelected(PlayerIndex playerIndex);

    enum class ElementState { Invisible, Appearing, Visible };

    bool isExiting_ = false;

    float maxEntryWidth_ = 0.0f;

    System::TimeSpan ninjaAppearDelay_ = System::TimeSpan::FromSeconds(3);
    System::TimeSpan titleAppearDelay_ = System::TimeSpan::FromSeconds(2);
    System::TimeSpan ninjaAppearDuration_ = System::TimeSpan::FromMilliseconds(500);
    System::TimeSpan titleAppearDuration_ = System::TimeSpan::FromMilliseconds(500);

    System::TimeSpan ninjaTimer_ = System::TimeSpan::Zero;
    System::TimeSpan titleTimer_ = System::TimeSpan::Zero;

    ElementState ninjaState_ = ElementState::Invisible;
    ElementState titleState_ = ElementState::Invisible;

    Texture2D instructionsTexture_;
    Texture2D loadingTexture_;

    Texture2D ninjaTexture_;
    Texture2D titleTexture_;

    Vector2 ninjaPosition_{30.0f, 40.0f};
    Vector2 titlePosition_{265.0f, 20.0f};

    Vector2 ninjaInitialOffset_{-400.0f, 50.0f};
    Vector2 titleInitialOffset_{0.0f, -280.0f};

    Vector2 ninjaOffset_ = ninjaInitialOffset_;
    Vector2 titleOffset_ = titleInitialOffset_;

    bool isMovingToLoading_ = false;
};

} // namespace NinjAcademy
