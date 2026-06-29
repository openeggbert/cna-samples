#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "System/Random.hpp"

#include "MenuScreen.hpp"

namespace GameStateManagement {

using Microsoft::Xna::Framework::Graphics::Texture2D;

// ===================== BackgroundScreen =====================
// Sits behind all the other menu screens, drawing a fixed background image.
class BackgroundScreen : public GameScreen {
public:
    BackgroundScreen() {
        setTransitionOnTime(TimeSpan::FromSeconds(0.5));
        setTransitionOffTime(TimeSpan::FromSeconds(0.5));
    }

    void LoadContent() override {
        backgroundTexture_.emplace(
            screenManager_->getGameProperty().getContentProperty().Load<Texture2D>("background"));
    }

    // Unlike most screens this should not transition off when covered.
    void Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override {
        (void)coveredByOtherScreen;
        GameScreen::Update(gameTime, otherScreenHasFocus, false);
    }

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        SpriteBatch& spriteBatch = screenManager_->getSpriteBatch();
        auto& viewport = screenManager_->getGraphicsDeviceProperty().getViewportProperty();
        Rectangle fullscreen(0, 0, viewport.getWidthProperty(), viewport.getHeightProperty());
        float a = TransitionAlpha();

        spriteBatch.Begin();
        spriteBatch.Draw(*backgroundTexture_, fullscreen, Color(a, a, a));
        spriteBatch.End();
    }

private:
    std::optional<Texture2D> backgroundTexture_;
};

// ===================== MessageBoxScreen =====================
// A popup message box, used to display "are you sure?" confirmation messages.
class MessageBoxScreen : public GameScreen {
public:
    std::function<void(PlayerIndex)> Accepted;
    std::function<void(PlayerIndex)> Cancelled;

    explicit MessageBoxScreen(const std::string& message, bool includeUsageText = true) {
        const std::string usageText = "\nA button, Space, Enter = ok"
                                      "\nB button, Esc = cancel";
        message_ = includeUsageText ? message + usageText : message;

        setIsPopup(true);
        setTransitionOnTime(TimeSpan::FromSeconds(0.2));
        setTransitionOffTime(TimeSpan::FromSeconds(0.2));
    }

    void LoadContent() override {
        gradientTexture_.emplace(
            screenManager_->getGameProperty().getContentProperty().Load<Texture2D>("gradient"));
    }

    void HandleInput(InputState& input) override {
        PlayerIndex playerIndex;
        if (input.IsMenuSelect(ControllingPlayer(), playerIndex)) {
            if (Accepted) Accepted(playerIndex);
            ExitScreen();
        } else if (input.IsMenuCancel(ControllingPlayer(), playerIndex)) {
            if (Cancelled) Cancelled(playerIndex);
            ExitScreen();
        }
    }

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        SpriteBatch& spriteBatch = screenManager_->getSpriteBatch();
        SpriteFont& font = screenManager_->getFont();

        // Darken down any other screens that were drawn beneath the popup.
        screenManager_->FadeBackBufferToBlack(TransitionAlpha() * 2 / 3);

        auto& viewport = screenManager_->getGraphicsDeviceProperty().getViewportProperty();
        Vector2 textSize = font.MeasureString(message_);
        Vector2 textPosition(((float)viewport.getWidthProperty() - textSize.X) / 2,
                             ((float)viewport.getHeightProperty() - textSize.Y) / 2);

        const int hPad = 32;
        const int vPad = 16;

        Rectangle backgroundRectangle((int)textPosition.X - hPad,
                                      (int)textPosition.Y - vPad,
                                      (int)textSize.X + hPad * 2,
                                      (int)textSize.Y + vPad * 2);

        Color color = mul(Color::White, TransitionAlpha());

        spriteBatch.Begin();
        spriteBatch.Draw(*gradientTexture_, backgroundRectangle, color);
        spriteBatch.DrawString(font, message_, textPosition, color);
        spriteBatch.End();
    }

private:
    std::string message_;
    std::optional<Texture2D> gradientTexture_;
};

// ===================== OptionsMenuScreen =====================
class OptionsMenuScreen : public MenuScreen {
public:
    OptionsMenuScreen() : MenuScreen("Options") {
        ungulateMenuEntry_ = std::make_shared<MenuEntry>("");
        languageMenuEntry_ = std::make_shared<MenuEntry>("");
        frobnicateMenuEntry_ = std::make_shared<MenuEntry>("");
        elfMenuEntry_ = std::make_shared<MenuEntry>("");

        SetMenuEntryText();

        auto back = std::make_shared<MenuEntry>("Back");

        ungulateMenuEntry_->Selected = [this](PlayerIndex) { currentUngulate_ = (currentUngulate_ + 1) % 3; SetMenuEntryText(); };
        languageMenuEntry_->Selected = [this](PlayerIndex) { currentLanguage_ = (currentLanguage_ + 1) % 3; SetMenuEntryText(); };
        frobnicateMenuEntry_->Selected = [this](PlayerIndex) { frobnicate_ = !frobnicate_; SetMenuEntryText(); };
        elfMenuEntry_->Selected = [this](PlayerIndex) { elf_++; SetMenuEntryText(); };
        back->Selected = [this](PlayerIndex p) { OnCancel(p); };

        MenuEntries().push_back(ungulateMenuEntry_);
        MenuEntries().push_back(languageMenuEntry_);
        MenuEntries().push_back(frobnicateMenuEntry_);
        MenuEntries().push_back(elfMenuEntry_);
        MenuEntries().push_back(back);
    }

private:
    void SetMenuEntryText() {
        static const char* ungulates[] = {"BactrianCamel", "Dromedary", "Llama"};
        static const char* languages[] = {"C#", "French", "Deoxyribonucleic acid"};
        ungulateMenuEntry_->setText(std::string("Preferred ungulate: ") + ungulates[currentUngulate_]);
        languageMenuEntry_->setText(std::string("Language: ") + languages[currentLanguage_]);
        frobnicateMenuEntry_->setText(std::string("Frobnicate: ") + (frobnicate_ ? "on" : "off"));
        elfMenuEntry_->setText(std::string("elf: ") + std::to_string(elf_));
    }

    std::shared_ptr<MenuEntry> ungulateMenuEntry_, languageMenuEntry_, frobnicateMenuEntry_, elfMenuEntry_;

    // Persisted between openings (matches the C# original's static fields).
    inline static int currentUngulate_ = 1;  // Dromedary
    inline static int currentLanguage_ = 0;
    inline static bool frobnicate_ = true;
    inline static int elf_ = 23;
};

// ===================== LoadingScreen =====================
class LoadingScreen : public GameScreen {
public:
    LoadingScreen(bool loadingIsSlow, std::vector<std::shared_ptr<GameScreen>> screensToLoad)
        : loadingIsSlow_(loadingIsSlow), screensToLoad_(std::move(screensToLoad)) {
        setTransitionOnTime(TimeSpan::FromSeconds(0.5));
    }

    // Activates the loading screen.
    static void Load(ScreenManager& screenManager, bool loadingIsSlow,
                     std::optional<PlayerIndex> controllingPlayer,
                     std::vector<std::shared_ptr<GameScreen>> screensToLoad) {
        for (auto& screen : screenManager.GetScreens())
            screen->ExitScreen();

        auto loadingScreen = std::make_shared<LoadingScreen>(loadingIsSlow, std::move(screensToLoad));
        screenManager.AddScreen(loadingScreen, controllingPlayer);
    }

    void Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override {
        GameScreen::Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);

        // 'this' stays alive: the ScreenManager update loop holds a shared_ptr copy.
        if (otherScreensAreGone_) {
            screenManager_->RemoveScreen(this);
            for (auto& screen : screensToLoad_)
                if (screen)
                    screenManager_->AddScreen(screen, ControllingPlayer());
            screenManager_->getGameProperty().ResetElapsedTime();
        }
    }

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        if (GetScreenState() == ScreenState::Active &&
            screenManager_->GetScreens().size() == 1) {
            otherScreensAreGone_ = true;
        }

        if (loadingIsSlow_) {
            SpriteBatch& spriteBatch = screenManager_->getSpriteBatch();
            SpriteFont& font = screenManager_->getFont();
            const std::string message = "Loading...";

            auto& viewport = screenManager_->getGraphicsDeviceProperty().getViewportProperty();
            Vector2 textSize = font.MeasureString(message);
            Vector2 textPosition(((float)viewport.getWidthProperty() - textSize.X) / 2,
                                 ((float)viewport.getHeightProperty() - textSize.Y) / 2);

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
class MainMenuScreen : public MenuScreen {
public:
    MainMenuScreen() : MenuScreen("Main Menu") {
        auto playGameMenuEntry = std::make_shared<MenuEntry>("Play Game");
        auto optionsMenuEntry = std::make_shared<MenuEntry>("Options");
        auto exitMenuEntry = std::make_shared<MenuEntry>("Exit");

        playGameMenuEntry->Selected = [this](PlayerIndex p) { PlayGameMenuEntrySelected(p); };
        optionsMenuEntry->Selected = [this](PlayerIndex p) {
            GetScreenManager()->AddScreen(std::make_shared<OptionsMenuScreen>(), p);
        };
        exitMenuEntry->Selected = [this](PlayerIndex p) { OnCancel(p); };

        MenuEntries().push_back(playGameMenuEntry);
        MenuEntries().push_back(optionsMenuEntry);
        MenuEntries().push_back(exitMenuEntry);
    }

protected:
    // When the user cancels the main menu, ask if they want to exit the sample.
    void OnCancel(PlayerIndex playerIndex) override {
        auto confirmExitMessageBox =
            std::make_shared<MessageBoxScreen>("Are you sure you want to exit this sample?");
        confirmExitMessageBox->Accepted = [this](PlayerIndex) {
            GetScreenManager()->getGameProperty().Exit();
        };
        GetScreenManager()->AddScreen(confirmExitMessageBox, playerIndex);
    }

private:
    // Defined at the bottom of this file (needs GameplayScreen + LoadingScreen).
    void PlayGameMenuEntrySelected(PlayerIndex playerIndex);
};

// ===================== GameplayScreen =====================
// Placeholder gameplay: the player and enemy are just text strings.
class GameplayScreen : public GameScreen {
public:
    GameplayScreen() {
        setTransitionOnTime(TimeSpan::FromSeconds(1.5));
        setTransitionOffTime(TimeSpan::FromSeconds(0.5));
    }

    void LoadContent() override {
        gameFont_.emplace(
            screenManager_->getGameProperty().getContentProperty().Load<SpriteFont>("gamefont"));

        // A real game would have more content; simulate a slow load.
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        screenManager_->getGameProperty().ResetElapsedTime();
    }

    void Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override {
        GameScreen::Update(gameTime, otherScreenHasFocus, false);

        if (coveredByOtherScreen)
            pauseAlpha_ = std::min(pauseAlpha_ + 1.0f / 32, 1.0f);
        else
            pauseAlpha_ = std::max(pauseAlpha_ - 1.0f / 32, 0.0f);

        if (IsActive()) {
            const float randomization = 10;
            enemyPosition_.X += (float)(random_.NextDouble() - 0.5) * randomization;
            enemyPosition_.Y += (float)(random_.NextDouble() - 0.5) * randomization;

            Vector2 targetPosition(
                (float)(screenManager_->getGraphicsDeviceProperty().getViewportProperty().getWidthProperty() / 2)
                    - gameFont_->MeasureString("Insert Gameplay Here").X / 2,
                200.0f);

            enemyPosition_ = Vector2::Lerp(enemyPosition_, targetPosition, 0.05f);
        }
    }

    // Defined at the bottom of this file (creates PauseMenuScreen).
    void HandleInput(InputState& input) override;

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        screenManager_->getGraphicsDeviceProperty().Clear(Color::CornflowerBlue);

        SpriteBatch& spriteBatch = screenManager_->getSpriteBatch();

        spriteBatch.Begin();
        spriteBatch.DrawString(*gameFont_, "// TODO", playerPosition_, Color::Green);
        spriteBatch.DrawString(*gameFont_, "Insert Gameplay Here", enemyPosition_, Color::DarkRed);
        spriteBatch.End();

        if (TransitionPosition() > 0 || pauseAlpha_ > 0) {
            float alpha = MathHelper::Lerp(1.0f - TransitionAlpha(), 1.0f, pauseAlpha_ / 2);
            screenManager_->FadeBackBufferToBlack(alpha);
        }
    }

private:
    std::optional<SpriteFont> gameFont_;
    Vector2 playerPosition_{100, 100};
    Vector2 enemyPosition_{100, 100};
    System::Random random_;
    float pauseAlpha_ = 0.0f;
};

// ===================== PauseMenuScreen =====================
class PauseMenuScreen : public MenuScreen {
public:
    PauseMenuScreen() : MenuScreen("Paused") {
        auto resumeGameMenuEntry = std::make_shared<MenuEntry>("Resume Game");
        auto quitGameMenuEntry = std::make_shared<MenuEntry>("Quit Game");

        resumeGameMenuEntry->Selected = [this](PlayerIndex p) { OnCancel(p); };
        quitGameMenuEntry->Selected = [this](PlayerIndex) {
            auto confirmQuitMessageBox =
                std::make_shared<MessageBoxScreen>("Are you sure you want to quit this game?");
            confirmQuitMessageBox->Accepted = [this](PlayerIndex) {
                std::vector<std::shared_ptr<GameScreen>> next;
                next.push_back(std::make_shared<BackgroundScreen>());
                next.push_back(std::make_shared<MainMenuScreen>());
                LoadingScreen::Load(*GetScreenManager(), false, std::nullopt, std::move(next));
            };
            GetScreenManager()->AddScreen(confirmQuitMessageBox, ControllingPlayer());
        };

        MenuEntries().push_back(resumeGameMenuEntry);
        MenuEntries().push_back(quitGameMenuEntry);
    }
};

// ---- cross-referencing method definitions (after all screens declared) ----

inline void MainMenuScreen::PlayGameMenuEntrySelected(PlayerIndex playerIndex) {
    std::vector<std::shared_ptr<GameScreen>> next;
    next.push_back(std::make_shared<GameplayScreen>());
    LoadingScreen::Load(*GetScreenManager(), true, playerIndex, std::move(next));
}

inline void GameplayScreen::HandleInput(InputState& input) {
    int playerIndex = static_cast<int>(ControllingPlayer().value());

    const KeyboardState& keyboardState = input.CurrentKeyboardStates[playerIndex];
    const GamePadState& gamePadState = input.CurrentGamePadStates[playerIndex];

    bool gamePadDisconnected = !gamePadState.getIsConnectedProperty() &&
                               input.GamePadWasConnected[playerIndex];

    if (input.IsPauseGame(ControllingPlayer()) || gamePadDisconnected) {
        screenManager_->AddScreen(std::make_shared<PauseMenuScreen>(), ControllingPlayer());
    } else {
        Vector2 movement = Vector2::Zero;

        if (keyboardState.IsKeyDown(Keys::Left))  movement.X--;
        if (keyboardState.IsKeyDown(Keys::Right)) movement.X++;
        if (keyboardState.IsKeyDown(Keys::Up))    movement.Y--;
        if (keyboardState.IsKeyDown(Keys::Down))  movement.Y++;

        Vector2 thumbstick = gamePadState.getThumbSticksProperty().getLeftProperty();
        movement.X += thumbstick.X;
        movement.Y -= thumbstick.Y;

        if (movement.Length() > 1)
            movement.Normalize();

        playerPosition_ = Vector2(playerPosition_.X + movement.X * 2,
                                  playerPosition_.Y + movement.Y * 2);
    }
}

} // namespace GameStateManagement
