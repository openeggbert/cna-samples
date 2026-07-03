#pragma once

#include <memory>
#include <sstream>
#include <string>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "System/Random.hpp"
#include "System/TimeSpan.hpp"

#include "PanelControl.hpp"
#include "ScrollingPanelControl.hpp"
#include "TextControl.hpp"

namespace UISample::Controls {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Content::ContentManager;
using Microsoft::Xna::Framework::Graphics::SpriteFont;

// Displays a list of high scores, as an example of presenting a list of data
// the player can scroll through. Port of Controls/HighScorePanel.cs.
class HighScorePanel : public ScrollingPanelControl {
public:
    explicit HighScorePanel(ContentManager& content)
        : titleFont_(content.Load<SpriteFont>("Font/MenuTitle")),
          headerFont_(content.Load<SpriteFont>("Font/MenuHeader")),
          detailFont_(content.Load<SpriteFont>("Font/MenuDetail")) {
        AddChild(std::make_shared<TextControl>("High score", &titleFont_));
        AddChild(CreateHeaderControl());
        PopulateWithFakeData();
    }

private:
    void PopulateWithFakeData() {
        auto newList = std::make_shared<PanelControl>();
        System::Random rng;
        for (int i = 0; i < 50; i++) {
            long score = 10000 - i * 10;
            System::TimeSpan time = System::TimeSpan::FromSeconds(rng.Next(60, 3600));
            newList->AddChild(CreateLeaderboardEntryControl("player" + std::to_string(i), score, time));
        }
        newList->LayoutColumn(0.0f, 0.0f, 0.0f);

        if (resultListControl_) {
            RemoveChild(resultListControl_);
        }
        resultListControl_ = newList;
        AddChild(resultListControl_);
        LayoutColumn(0.0f, 0.0f, 0.0f);
    }

    std::shared_ptr<Control> CreateHeaderControl() {
        auto panel = std::make_shared<PanelControl>();
        panel->AddChild(std::make_shared<TextControl>("Player", &headerFont_, Color::Turquoise, Vector2(0.0f, 0.0f)));
        panel->AddChild(std::make_shared<TextControl>("Score", &headerFont_, Color::Turquoise, Vector2(200.0f, 0.0f)));
        return panel;
    }

    // Creates a Control to display one leaderboard entry. The content is
    // broken into parameters so a control can easily be created with fake
    // data when running without a real leaderboard service.
    std::shared_ptr<Control> CreateLeaderboardEntryControl(const std::string& player, long rating,
                                                           System::TimeSpan time) {
        Color textColor = Color::White;
        auto panel = std::make_shared<PanelControl>();

        auto playerText = std::make_shared<TextControl>();
        playerText->setText(player);
        playerText->setFont(&detailFont_);
        playerText->TextColor = textColor;
        playerText->setPosition(Vector2(0.0f, 0.0f));
        panel->AddChild(playerText);

        auto scoreText = std::make_shared<TextControl>();
        scoreText->setText(std::to_string(rating));
        scoreText->setFont(&detailFont_);
        scoreText->TextColor = textColor;
        scoreText->setPosition(Vector2(200.0f, 0.0f));
        panel->AddChild(scoreText);

        auto timeText = std::make_shared<TextControl>();
        timeText->setText("Completed in " + FormatTime(time));
        timeText->setFont(&detailFont_);
        timeText->TextColor = textColor;
        timeText->setPosition(Vector2(400.0f, 0.0f));
        panel->AddChild(timeText);

        return panel;
    }

    // The original formats the TimeSpan with .NET's general TimeSpan format
    // ("{0:g}", e.g. "1:02:03"), which sharp-runtime's String::Format doesn't
    // implement (only Format's own supported specifiers apply to numeric
    // types, not TimeSpan). Manually formatted instead -- same class of
    // adaptation as SnowShovel's FormatMinutesSeconds/Platformer's pad2.
    static std::string FormatTime(const System::TimeSpan& time) {
        auto pad2 = [](int v) { return (v < 10 ? std::string("0") : std::string("")) + std::to_string(v); };
        int hours = (int)time.getTotalHoursProperty();
        return std::to_string(hours) + ":" + pad2(time.getMinutesProperty()) + ":" + pad2(time.getSecondsProperty());
    }

    SpriteFont titleFont_;
    SpriteFont headerFont_;
    SpriteFont detailFont_;
    std::shared_ptr<Control> resultListControl_;
};

} // namespace UISample::Controls
