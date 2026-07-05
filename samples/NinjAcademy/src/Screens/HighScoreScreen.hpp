#pragma once

// HighScoreScreen.hpp — C++ port of Screens/HighScoreScreen.cs (XNA 4.0
// NinjAcademy sample). Persistence uses a plain text file instead of
// IsolatedStorageFile, which has no CNA equivalent -- matching this
// project's established HoneycombRush precedent; see missing.md.
//
// Also defines NameEntryScreen (CNA addition, not in the original): the
// original asks for the player's name via Guide.BeginShowKeyboardInput,
// which CNA's Guide always completes with an empty string (no real
// system keyboard exists on desktop) -- see missing.md. This popup screen
// substitutes a simple keyboard-driven name entry (letter keys, Backspace,
// Enter) so the high-score name flow still works end to end.

#include <algorithm>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/KeyboardState.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"

#include "../AudioManager.hpp"
#include "../GameConstants.hpp"
#include "../ScreenManager/GameScreen.hpp"
#include "BackgroundScreen.hpp"
#include "MainMenuScreen.hpp"

namespace NinjAcademy {

using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Input::Keyboard;
using Microsoft::Xna::Framework::Input::Keys;

// Port of Screens/HighScoreScreen.cs.
class HighScoreScreen : public GameScreen {
public:
    static const int HighscorePlaces = 7;

    static std::vector<std::pair<std::string, int>>& HighScore() {
        static std::vector<std::pair<std::string, int>> highScore = {
            {"Goku", 9001},  {"Ellen", 500},  {"Terry", 250},     {"Dave", 100},
            {"Biff", 50},    {"Michael", 20}, {"Dan Hibiki", 10},
        };
        return highScore;
    }

    static bool& HighscoreLoaded() {
        static bool loaded = false;
        return loaded;
    }
    static bool& HighscoreSaved() {
        static bool saved = false;
        return saved;
    }

    HighScoreScreen() { setEnabledGestures(GestureType::Tap); }

    void LoadContent() override {
        highScoreFont_.emplace(Load<SpriteFont>("Fonts/HighScoreFont"));
        highScoreTitleTexture_.emplace(Load<Texture2D>("Textures/highscore_title"));

        viewport_ = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty().getBoundsProperty();
        titlePosition_ = Vector2((float)(viewport_.getCenterProperty().X - highScoreTitleTexture_->getWidthProperty() / 2),
                                 (float)GameConstants::HighScoreTitleTopMargin);
    }

    void HandleInput(InputState& input) override {
        if (input.IsPauseGame(std::nullopt)) {
            Exit();
        }

        // Return to the main menu when a tap gesture is recognized.
        if (!input.Gestures.empty()) {
            if (input.Gestures[0].getGestureTypeProperty() == GestureType::Tap) {
                Exit();
                input.Gestures.clear();
            }
        }
    }

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;

        SpriteBatch& spriteBatch = GetScreenManager()->getSpriteBatch();
        Vector2 textShadowVector(4.0f, 4.0f);

        spriteBatch.Begin();

        spriteBatch.Draw(*highScoreTitleTexture_, titlePosition_, Color::White);

        auto& highScore = HighScore();
        for (size_t i = 0; i < highScore.size(); i++) {
            if (highScore[i].first.empty())
                continue;

            Vector2 textPosition((float)GameConstants::HighScorePlaceLeftMargin,
                                 (float)(i * GameConstants::HighScoreVerticalJump + GameConstants::HighScoreTopMargin));

            spriteBatch.DrawString(*highScoreFont_, GetPlaceString((int)i), textPosition + textShadowVector, Color::Black);
            spriteBatch.DrawString(*highScoreFont_, GetPlaceString((int)i), textPosition, Color::White);

            textPosition.X = (float)GameConstants::HighScoreNameLeftMargin;
            spriteBatch.DrawString(*highScoreFont_, highScore[i].first, textPosition + textShadowVector, Color::Black);
            spriteBatch.DrawString(*highScoreFont_, highScore[i].first, textPosition, Color::White);

            textPosition.X = (float)GameConstants::HighScoreScoreLeftMargin;
            std::string scoreStr = std::to_string(highScore[i].second);
            spriteBatch.DrawString(*highScoreFont_, scoreStr, textPosition + textShadowVector, Color::Black);
            spriteBatch.DrawString(*highScoreFont_, scoreStr, textPosition, Color::White);
        }

        spriteBatch.End();
    }

    // Checks whether a score belongs on the high-score table.
    static bool IsInHighscores(int score) { return score > HighScore()[HighscorePlaces - 1].second; }

    // Puts a score on the high-score table, if it belongs there.
    static void PutHighScore(const std::string& playerName, int score) {
        if (IsInHighscores(score)) {
            HighScore()[HighscorePlaces - 1] = {playerName, score};
            OrderGameScore();
            SaveHighscore();
        }
    }

    static void HighScoreChanged() { HighscoreSaved() = false; }

    // Saves the current high scores to a text file (CNA has no IsolatedStorageFile; see missing.md).
    static void SaveHighscore() {
        std::ofstream out(HighScoreFilename());
        for (auto& entry : HighScore())
            out << entry.first << "\n" << entry.second << "\n";
        HighscoreSaved() = true;
    }

    // Loads the high scores from a text file, if one exists.
    static void LoadHighscores() {
        std::ifstream in(HighScoreFilename());
        if (in) {
            auto& highScore = HighScore();
            std::string name, scoreLine;
            size_t i = 0;
            while (i < highScore.size() && std::getline(in, name) && std::getline(in, scoreLine))
                highScore[i++] = {name, std::stoi(scoreLine)};
        }

        OrderGameScore();
        HighscoreLoaded() = true;
    }

private:
    static const std::string& HighScoreFilename() {
        static const std::string name = "highscores.txt";
        return name;
    }

    static void OrderGameScore() {
        auto& highScore = HighScore();
        std::sort(highScore.begin(), highScore.end(),
                  [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
                      return a.second > b.second;
                  });
    }

    void Exit() {
        ExitScreen();
        GetScreenManager()->AddScreen(std::make_shared<BackgroundScreen>("titlescreenBG"), std::nullopt);
        GetScreenManager()->AddScreen(std::make_shared<MainMenuScreen>(), std::nullopt);
    }

    static std::string GetPlaceString(int number) {
        static const char* places[HighscorePlaces] = {"1.", "2.", "3.", "4.", "5.", "6.", "7."};
        return places[number];
    }

    std::optional<SpriteFont> highScoreFont_;

    std::optional<Texture2D> highScoreTitleTexture_;
    Vector2 titlePosition_;

    Rectangle viewport_;
};

// CNA addition (see file header): a popup screen for entering the player's
// name after a new high score, substituting Guide.BeginShowKeyboardInput
// (which CNA cannot fulfil with real text on this desktop). Exits all
// current screens and shows the high-score table once a name is confirmed.
class NameEntryScreen : public GameScreen {
public:
    explicit NameEntryScreen(int score) : score_(score) {
        setIsPopup(true);
        setTransitionOnTime(System::TimeSpan::FromSeconds(0.3));
        setTransitionOffTime(System::TimeSpan::FromSeconds(0.3));
    }

    void LoadContent() override { font_.emplace(Load<SpriteFont>("Fonts/GameScreenFont28px")); }

    void HandleInput(InputState& input) override {
        (void)input;

        KeyboardState current = Keyboard::GetState();

        for (int k = (int)Keys::A; k <= (int)Keys::Z; k++) {
            Keys key = (Keys)k;
            bool down = current.IsKeyDown(key);
            bool wasDown = lastKeyboard_.IsKeyDown(key);
            if (down && !wasDown && name_.size() < MaxNameLength) {
                char c = (char)('A' + (k - (int)Keys::A));
                name_.push_back(c);
            }
        }

        bool backDown = current.IsKeyDown(Keys::Back);
        if (backDown && !lastKeyboard_.IsKeyDown(Keys::Back) && !name_.empty()) {
            name_.pop_back();
        }

        bool enterDown = current.IsKeyDown(Keys::Enter);
        if (enterDown && !lastKeyboard_.IsKeyDown(Keys::Enter)) {
            std::string playerName = name_.empty() ? std::string("Player") : name_;
            HighScoreScreen::PutHighScore(playerName, score_);

            for (auto& screen : GetScreenManager()->GetScreens())
                screen->ExitScreen();

            GetScreenManager()->AddScreen(std::make_shared<BackgroundScreen>("highScoreBG"), std::nullopt);
            GetScreenManager()->AddScreen(std::make_shared<HighScoreScreen>(), std::nullopt);
        }

        lastKeyboard_ = current;
    }

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        SpriteBatch& spriteBatch = GetScreenManager()->getSpriteBatch();
        auto& viewport = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty();

        const std::string title = "A new high-score!";
        const std::string prompt = "Enter your name (Enter to confirm):";

        Vector2 titleSize = font_->MeasureString(title);
        Vector2 center((float)(viewport.getWidthProperty() / 2), (float)(viewport.getHeightProperty() / 2));

        spriteBatch.Begin();
        GetScreenManager()->FadeBackBufferToBlack(TransitionAlpha() * 2.0f / 3.0f);

        spriteBatch.DrawString(*font_, title, center - Vector2(titleSize.X / 2.0f, 60.0f), Color::White);
        spriteBatch.DrawString(*font_, prompt, center - Vector2(font_->MeasureString(prompt).X / 2.0f, 20.0f),
                               Color::White);
        spriteBatch.DrawString(*font_, name_, center - Vector2(font_->MeasureString(name_).X / 2.0f, -20.0f),
                               Color::Yellow);
        spriteBatch.End();
    }

private:
    static const size_t MaxNameLength = 25;

    int score_;
    std::string name_;
    std::optional<SpriteFont> font_;
    KeyboardState lastKeyboard_;
};

} // namespace NinjAcademy
