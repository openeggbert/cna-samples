#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"

#include "GameScreen.hpp"
#include "MenuScreen.hpp"
#include "ScreenManager.hpp"
#include "Controls/Control.hpp"
#include "Controls/HighScorePanel.hpp"
#include "Controls/ImageControl.hpp"
#include "Controls/PageFlipControl.hpp"
#include "Controls/PageFlipTracker.hpp"
#include "Controls/ScrollTracker.hpp"
#include "Controls/TextControl.hpp"

namespace UISample {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Content::ContentManager;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Input::Buttons;
using Controls::Control;
using Controls::HighScorePanel;
using Controls::ImageControl;
using Controls::PageFlipControl;
using Controls::PageFlipTracker;
using Controls::PanelControl;
using Controls::ScrollTracker;
using Controls::TextControl;

// ===================== BackgroundScreen =====================
// Sits behind all the other menu screens, drawing a fixed background image
// that stays put regardless of whatever transitions the screens on top of it
// may be doing. Port of Screens/BackgroundScreen.cs (simplified to use the
// shared ContentManager rather than a private one — see missing.md).
class BackgroundScreen : public GameScreen {
public:
    BackgroundScreen() {
        setTransitionOnTime(System::TimeSpan::FromSeconds(0.5));
        setTransitionOffTime(System::TimeSpan::FromSeconds(0.5));
    }

    void LoadContent() override {
        backgroundTexture_.emplace(
            GetScreenManager()->getGameProperty().getContentProperty().Load<Texture2D>("background"));
    }

    // Unlike most screens, this should not transition off even when covered:
    // it's supposed to be covered, after all. Forces coveredByOtherScreen to
    // false so the base Update doesn't try to transition off.
    void Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override {
        (void)coveredByOtherScreen;
        GameScreen::Update(gameTime, otherScreenHasFocus, false);
    }

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        auto& spriteBatch = GetScreenManager()->getSpriteBatch();
        auto& viewport = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty();
        Rectangle fullscreen(0, 0, viewport.getWidthProperty(), viewport.getHeightProperty());

        spriteBatch.Begin();
        float a = TransitionAlpha();
        spriteBatch.Draw(*backgroundTexture_, fullscreen, Color(a, a, a));
        spriteBatch.End();
    }

private:
    std::optional<Texture2D> backgroundTexture_;
};

// ===================== SingleControlScreen =====================
// A screen containing a single Control. Bridges the 'Controls' UI system and
// the 'ScreenManager' screen system. Port of Screens/SingleControlScreen.cs.
class SingleControlScreen : public GameScreen {
public:
    void Draw(const GameTime& gameTime) override {
        if (rootControl_) {
            Control::BatchDraw(rootControl_.get(), GetScreenManager()->getGraphicsDeviceProperty(),
                               GetScreenManager()->getSpriteBatch(), GetScreenManager()->getBlankTexture(),
                               Vector2::Zero, gameTime);
        }
    }

    void Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override {
        if (rootControl_) rootControl_->Update(gameTime);
        GameScreen::Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);
    }

    void HandleInput(InputState& input) override {
        // cancel the current screen if the user presses the back button
        PlayerIndex player;
        if (input.IsNewButtonPress(Buttons::Back, std::nullopt, player)) {
            ExitScreen();
        }

        if (rootControl_) rootControl_->HandleInput(input);
    }

protected:
    // The sole Control in this screen. Derived classes can do what they like with it.
    std::shared_ptr<Control> rootControl_;
};

// ===================== LoadingScreen =====================
// Coordinates transitions between the menu system and the game itself.
// Normally one screen transitions off at the same time as the next
// transitions on, but for transitions that need to load a lot of data, the
// menu system should be entirely gone before the load starts:
// - Tell all existing screens to transition off.
// - Activate a loading screen, which transitions on at the same time.
// - The loading screen watches the state of the previous screens.
// - When they've finished transitioning off, activate the real next screen,
//   which may take a while to load. The loading screen is the only thing
//   displayed while that load happens.
// Port of Screens/LoadingScreen.cs (tombstoning/IsSerializable dropped, see
// missing.md).
class LoadingScreen : public GameScreen {
public:
    // The constructor is private-equivalent: loading screens should be
    // activated via the static Load method instead.
    static void Load(ScreenManager& screenManager, bool loadingIsSlow,
                     std::optional<PlayerIndex> controllingPlayer,
                     std::vector<std::shared_ptr<GameScreen>> screensToLoad) {
        // Tell all the current screens to transition off.
        for (auto& screen : screenManager.GetScreens()) {
            screen->ExitScreen();
        }

        auto loadingScreen = std::make_shared<LoadingScreen>(loadingIsSlow, std::move(screensToLoad));
        screenManager.AddScreen(loadingScreen, controllingPlayer);
    }

    LoadingScreen(bool loadingIsSlow, std::vector<std::shared_ptr<GameScreen>> screensToLoad)
        : loadingIsSlow_(loadingIsSlow), screensToLoad_(std::move(screensToLoad)) {
        setTransitionOnTime(System::TimeSpan::FromSeconds(0.5));
    }

    void Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override {
        GameScreen::Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);

        // If all the previous screens have finished transitioning off, it's
        // time to actually perform the load.
        if (otherScreensAreGone_) {
            GetScreenManager()->RemoveScreen(this);

            for (auto& screen : screensToLoad_) {
                if (screen) {
                    GetScreenManager()->AddScreen(screen, ControllingPlayer());
                }
            }

            // Once the load has finished, tell the game timing mechanism we
            // just finished a very long frame and should not try to catch up.
            GetScreenManager()->getGameProperty().ResetElapsedTime();
        }
    }

    void Draw(const GameTime& gameTime) override {
        // If we're the only active screen, all the previous screens must
        // have finished transitioning off. Checked in Draw rather than
        // Update because it isn't enough for the screens to be gone: for the
        // transition to look good we must have actually drawn a frame
        // without them before performing the load.
        if (GetScreenState() == ScreenState::Active && GetScreenManager()->GetScreens().size() == 1) {
            otherScreensAreGone_ = true;
        }

        // The gameplay screen takes a while to load, so a loading message is
        // shown while that happens; the menus load quickly, so it would look
        // silly to flash this up for a fraction of a second when returning
        // from the game to the menus.
        if (loadingIsSlow_) {
            auto& spriteBatch = GetScreenManager()->getSpriteBatch();
            auto& font = GetScreenManager()->getFont();

            const std::string message = "Loading...";

            auto& viewport = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty();
            Vector2 viewportSize((float)viewport.getWidthProperty(), (float)viewport.getHeightProperty());
            Vector2 textSize = font.MeasureString(message);
            Vector2 textPosition = (viewportSize - textSize) / 2.0f;

            Color color = mul(Color::White, TransitionAlpha());

            spriteBatch.Begin();
            spriteBatch.DrawString(font, message, textPosition, color);
            spriteBatch.End();
        }
    }

private:
    bool loadingIsSlow_;
    bool otherScreensAreGone_ = false;
    std::vector<std::shared_ptr<GameScreen>> screensToLoad_;
};

// ===================== MainMenuScreen =====================
// The first thing displayed when the game starts up. Port of
// Screens/MainMenuScreen.cs.
class MainMenuScreen : public MenuScreen {
public:
    MainMenuScreen() : MenuScreen("Main Menu") {
        auto levelSelect = std::make_shared<MenuEntry>("Select level");
        levelSelect->Selected = [this](PlayerIndex p) { SelectLevelPressed(p); };
        MenuEntries().push_back(levelSelect);

        auto highScores = std::make_shared<MenuEntry>("High scores");
        highScores->Selected = [this](PlayerIndex p) { HighScoresPressed(p); };
        MenuEntries().push_back(highScores);
    }

protected:
    void OnCancel(PlayerIndex playerIndex) override {
        (void)playerIndex;
        GetScreenManager()->getGameProperty().Exit();
    }

private:
    // The level select screen needs to load a decent amount of level art, so
    // the loading screen is used to move there. Load() exits all current
    // screens, so the background and main menu screens are passed down too,
    // to make it easy to come back from level select.
    void SelectLevelPressed(PlayerIndex playerIndex);

    // Defined out-of-line below, once HighScoreScreen is a complete type.
    void HighScoresPressed(PlayerIndex playerIndex);
};

// ===================== LevelSelectScreen =====================
struct LevelInfo {
    std::string Name;
    std::string Description;
    std::string Image;
};

// A control that draws one level's background image, title, and description.
class LevelDescriptionPanel : public PanelControl {
public:
    LevelDescriptionPanel(ContentManager& content, const LevelInfo& info) {
        backgroundTexture_ = content.Load<Texture2D>(info.Image);
        auto background = std::make_shared<ImageControl>(&*backgroundTexture_, Vector2::Zero);
        AddChild(background);

        titleFont_ = content.Load<SpriteFont>("Font/MenuTitle");
        auto title = std::make_shared<TextControl>(info.Name, &*titleFont_, Color::Black,
                                                    Vector2(MarginLeft, MarginTop));
        AddChild(title);

        descriptionFont_ = content.Load<SpriteFont>("Font/MenuDetail");
        auto description = std::make_shared<TextControl>(info.Description, &*descriptionFont_, Color::Black,
                                                          Vector2(MarginLeft, DescriptionTop));
        AddChild(description);
    }

private:
    static constexpr float MarginLeft = 20.0f;
    static constexpr float MarginTop = 20.0f;
    static constexpr float DescriptionTop = 440.0f;

    std::optional<Texture2D> backgroundTexture_;
    std::optional<SpriteFont> titleFont_;
    std::optional<SpriteFont> descriptionFont_;
};

// Demonstrates PageFlipControl by letting the player choose from a set of
// game levels. Port of Screens/LevelSelectScreen.cs.
class LevelSelectScreen : public SingleControlScreen {
public:
    void LoadContent() override {
        setEnabledGestures(PageFlipTracker::GesturesNeeded);
        auto& content = GetScreenManager()->getGameProperty().getContentProperty();

        auto pageFlip = std::make_shared<PageFlipControl>();

        static const LevelInfo levelInfos[] = {
            {"House", "Find a way out of your house--if you dare!", "Levels/House"},
            {"Pasture", "Locate your magical cow", "Levels/Pasture"},
            {"Hills", "Graze across the hills", "Levels/Hills"},
            {"Castle", "Explore the old ruined castle", "Levels/Castle"},
            {"Dungeon", "Conquer the dreaded Dungeon Critter", "Levels/Dungeon"},
        };

        for (const LevelInfo& info : levelInfos) {
            pageFlip->AddChild(std::make_shared<LevelDescriptionPanel>(content, info));
        }

        rootControl_ = pageFlip;
    }
};

// ===================== HighScoreScreen =====================
// Creates a single PageFlipControl... actually a single HighScorePanel
// wrapped in a SingleControlScreen, displaying fake leaderboard data. Port of
// Screens/HighScoreScreen.cs.
class HighScoreScreen : public SingleControlScreen {
public:
    void LoadContent() override {
        setEnabledGestures(ScrollTracker::GesturesNeeded);
        auto& content = GetScreenManager()->getGameProperty().getContentProperty();

        rootControl_ = std::make_shared<HighScorePanel>(content);
    }
};

// ---- cross-referencing method definitions (after all screens declared) ----

inline void MainMenuScreen::SelectLevelPressed(PlayerIndex playerIndex) {
    std::vector<std::shared_ptr<GameScreen>> screens;
    screens.push_back(std::make_shared<BackgroundScreen>());
    screens.push_back(std::make_shared<MainMenuScreen>());
    screens.push_back(std::make_shared<LevelSelectScreen>());
    LoadingScreen::Load(*GetScreenManager(), true, playerIndex, std::move(screens));
}

inline void MainMenuScreen::HighScoresPressed(PlayerIndex playerIndex) {
    GetScreenManager()->AddScreen(std::make_shared<HighScoreScreen>(), playerIndex);
}

} // namespace UISample
