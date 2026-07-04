#pragma once

// HighScoreScreen.hpp — C++ port of Screens/HighScoreScreen.cs (XNA 4.0
// HoneycombRush sample; the original file is oddly named "LoadingScreen.cs"
// on disk). Persistence uses a plain text file instead of
// IsolatedStorageFile, which has no CNA equivalent — see missing.md.

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureType.hpp"

#include "../ScreenManager/GameScreen.hpp"
#include "../ScreenManager/ScreenManager.hpp"
#include "BackgroundScreen.hpp"
#include "MainMenuScreen.hpp"

namespace HoneycombRush {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Input::Touch::GestureType;

// Displays the high score table and persists it to disk. Port of
// Screens/HighScoreScreen.cs.
class HighScoreScreen : public GameScreen {
public:
    static const int HighscorePlaces = 5;

    static std::vector<std::pair<std::string, int>>& HighScore() {
        static std::vector<std::pair<std::string, int>> highScore = {
            {"Jasper", 55000}, {"Ellen", 52750}, {"Terry", 52200}, {"Lori", 50200}, {"Michael", 50750},
        };
        return highScore;
    }

    HighScoreScreen() { setEnabledGestures(GestureType::Tap); }

    void LoadContent() override {
        highScoreFont_.emplace(Load<SpriteFont>("Fonts/HighScoreFont"));

        GameScreen::LoadContent();
    }

    void HandleInput(GameTime& gameTime, InputState& input) override {
        (void)gameTime;

        if (input.IsPauseGame(std::nullopt)) {
            Exit();
        }

        // Return to the main menu when a tap gesture is recognized.
        if (!input.Gestures.empty()) {
            const GestureSample& sample = input.Gestures[0];
            if (sample.getGestureTypeProperty() == GestureType::Tap) {
                Exit();
                input.Gestures.clear();
            }
        }
    }

    void Draw(const GameTime& gameTime) override {
        SpriteBatch& spriteBatch = GetScreenManager()->getSpriteBatch();

        spriteBatch.Begin();

        auto& highScore = HighScore();
        for (std::size_t i = 0; i < highScore.size(); i++) {
            if (!highScore[i].first.empty()) {
                spriteBatch.DrawString(*highScoreFont_, GetPlaceString((int)i), Vector2(20, (float)(i * 72 + 86)),
                                        Color::Black);
                spriteBatch.DrawString(*highScoreFont_, highScore[i].first, Vector2(210, (float)(i * 72 + 86)),
                                        Color::DarkRed);
                spriteBatch.DrawString(*highScoreFont_, std::to_string(highScore[i].second),
                                        Vector2(560, (float)(i * 72 + 86)), Color::Yellow);
            }
        }

        spriteBatch.End();

        GameScreen::Draw(gameTime);
    }

    // Checks whether a score belongs on the high score table.
    static bool IsInHighscores(int score) { return score > HighScore()[HighscorePlaces - 1].second; }

    // Puts a score on the high score table, if it belongs there.
    static void PutHighScore(const std::string& playerName, int score) {
        if (IsInHighscores(score)) {
            HighScore()[HighscorePlaces - 1] = {playerName, score};
            OrderGameScore();
            SaveHighscore();
        }
    }

    // Saves the current high scores to a text file (CNA has no
    // IsolatedStorageFile; see missing.md).
    static void SaveHighscore() {
        std::ofstream out(HighScoreFilename());
        for (auto& entry : HighScore())
            out << entry.first << "\n" << entry.second << "\n";
    }

    // Loads the high scores from a text file, if one exists.
    static void LoadHighscores() {
        std::ifstream in(HighScoreFilename());
        if (in) {
            auto& highScore = HighScore();
            std::string name, scoreLine;
            std::size_t i = 0;
            while (i < highScore.size() && std::getline(in, name) && std::getline(in, scoreLine))
                highScore[i++] = {name, std::stoi(scoreLine)};
        }

        OrderGameScore();
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
        GetScreenManager()->AddScreen(std::make_shared<BackgroundScreen>("titleScreen"), std::nullopt);
        GetScreenManager()->AddScreen(std::make_shared<MainMenuScreen>(), std::nullopt);
    }

    static std::string GetPlaceString(int number) {
        static const char* places[HighscorePlaces] = {"1ST", "2ND", "3RD", "4TH", "5TH"};
        return places[number];
    }

    std::optional<SpriteFont> highScoreFont_;
};

// ---- MainMenuScreen methods that depend on HighScoreScreen (defined here) ----

inline void MainMenuScreen::OnCancel(PlayerIndex playerIndex) {
    (void)playerIndex;

    HighScoreScreen::SaveHighscore();

    GetScreenManager()->getGameProperty().Exit();

    AudioManager::StopSound("MenuMusic_Loop");
}

} // namespace HoneycombRush
