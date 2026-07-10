#pragma once

// HighScoreScreen.hpp — C++ port of Screens/HighScoreScreen.cs (XNA 4.0
// MarbleMaze sample). Deviation (see missing.md): the original persists scores
// via Windows Phone's IsolatedStorageFile; CNA/sharp-runtime has no equivalent,
// so this port uses plain std::fstream against a "highscores.txt" file next to
// the built binary (this repo's samples are always run from their own binary
// directory -- NEXT.md section 6 -- so this is a stable, predictable location).

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureType.hpp"
#include "System/TimeSpan.hpp"

#include "../ScreenManager/GameScreen.hpp"
#include "../ScreenManager/ScreenManager.hpp"

namespace MarbleMazeSample {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Input::Touch::GestureType;
using System::TimeSpan;

class MainMenuScreen; // forward declaration -- Exit()'s body (deferred to
                       // Screens/ScreensGlue.hpp) constructs one.

class HighScoreScreen : public GameScreen {
public:
    static constexpr int HighscorePlaces = 10;

    struct Entry {
        std::string Name;
        TimeSpan Value;
    };

    static std::vector<Entry>& HighScores() { return highScore_; }

    HighScoreScreen() { setEnabledGestures(GestureType::Tap); }

    void LoadContent() override {
        highScoreFont_ = Load<SpriteFont>("Fonts/MenuFont");
        GameScreen::LoadContent();
    }

    // Deferred to Screens/ScreensGlue.hpp -- constructs MainMenuScreen.
    void HandleInput(InputState& input) override;

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        SpriteBatch& spriteBatch = GetScreenManager()->getSpriteBatch();

        spriteBatch.Begin();

        spriteBatch.DrawString(*highScoreFont_, "High Scores", Vector2(30, 30), Color::White);

        for (std::size_t i = 0; i < highScore_.size(); i++) {
            spriteBatch.DrawString(*highScoreFont_, std::to_string(i + 1) + ". " + highScore_[i].Name,
                                    Vector2(100, (float)i * 40 + 70), Color::YellowGreen);

            int minutes = (int)highScore_[i].Value.getMinutesProperty();
            int seconds = (int)highScore_[i].Value.getSecondsProperty();
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%02d:%02d", minutes, seconds);
            spriteBatch.DrawString(*highScoreFont_, buf, Vector2(500, (float)i * 40 + 70), Color::YellowGreen);
        }

        spriteBatch.End();
    }

    // Check if a score belongs on the high score table.
    static bool IsInHighscores(TimeSpan gameTime) { return gameTime < highScore_[HighscorePlaces - 1].Value; }

    // Put high score on the highscores table.
    static void PutHighScore(const std::string& playerName, TimeSpan gameTime) {
        if (IsInHighscores(gameTime)) {
            highScore_[HighscorePlaces - 1] = Entry{playerName, gameTime};
            OrderGameScore();
        }
    }

    static void SaveHighscore() {
        std::ofstream out("highscores.txt", std::ios::trunc);
        if (!out) return;
        for (const auto& entry : highScore_) {
            out << entry.Name << '\n' << (long long)entry.Value.getTicksProperty() << '\n';
        }
    }

    static void LoadHighscore() {
        std::ifstream in("highscores.txt");
        if (in) {
            std::vector<Entry> loaded;
            std::string name;
            std::string ticksLine;
            while (std::getline(in, name) && std::getline(in, ticksLine)) {
                long long ticks = std::stoll(ticksLine);
                loaded.push_back(Entry{name, TimeSpan::FromTicks(ticks)});
            }
            if (loaded.size() == HighscorePlaces) {
                highScore_ = std::move(loaded);
            }
        }
        OrderGameScore();
    }

private:
    void Exit();

    static void OrderGameScore() {
        std::sort(highScore_.begin(), highScore_.end(),
                  [](const Entry& a, const Entry& b) { return a.Value.getTicksProperty() < b.Value.getTicksProperty(); });
    }

    static inline std::vector<Entry> highScore_ = {
        {"Jasper", TimeSpan::FromSeconds(90)},  {"Ellen", TimeSpan::FromSeconds(110)},
        {"Terry", TimeSpan::FromSeconds(130)},  {"Lori", TimeSpan::FromSeconds(150)},
        {"Michael", TimeSpan::FromSeconds(170)}, {"Carol", TimeSpan::FromSeconds(190)},
        {"Toni", TimeSpan::FromSeconds(210)},   {"Cassie", TimeSpan::FromSeconds(230)},
        {"Luca", TimeSpan::FromSeconds(250)},   {"Brian", TimeSpan::FromSeconds(270)},
    };

    std::optional<SpriteFont> highScoreFont_;
};

} // namespace MarbleMazeSample
