#pragma once

// Hud.hpp -- simplified adaptation of GameScreens/Hud.cs. The original draws a
// rich bottom-of-screen strip: a scrollable character-portrait carousel with
// selection brackets, per-slot "active/inactive/can't-use" plank backgrounds,
// a combat action-menu popup, and Y/Start button-prompt icons (~700 lines,
// ~17 textures). This port keeps its public surface (ActionText, LoadContent,
// Draw) but renders a plain text readout instead -- party gold and each
// player's name/HP/MP -- since the elaborate portrait-carousel art assets
// are a presentation layer, not gameplay logic. See missing.md.

#include <string>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"

#include "../Fonts.hpp"
#include "../ScreenManager/ScreenManager.hpp"
#include "../Session/Session.hpp"

namespace RolePlaying {

class Hud {
public:
    static constexpr int HudHeight = 90;

    explicit Hud(ScreenManager& screenManager) : screenManager_(&screenManager) {}

    const std::string& ActionText() const { return actionText_; }
    void SetActionText(const std::string& v) { actionText_ = v; }

    void LoadContent() {}

    void Draw() {
        Party* party = Session::GetParty();
        if (!party) return;

        auto& spriteBatch = screenManager_->getSpriteBatch();
        auto viewportHeight = screenManager_->getGraphicsDeviceProperty().getViewportProperty().getHeightProperty();
        float y = (float)(viewportHeight - HudHeight);

        spriteBatch.Begin();
        spriteBatch.DrawString(Fonts::HudDetailFont(), "Gold: " + Fonts::GetGoldString(party->PartyGold()),
                               Microsoft::Xna::Framework::Vector2(10.0f, y), Fonts::DisplayColor);
        float x = 10.0f;
        for (auto& player : party->Players) {
            std::string line = player->Name() + "  HP:" + std::to_string(player->CurrentStatistics().HealthPoints) +
                                "/" + std::to_string(player->CharacterStatistics().HealthPoints) +
                                "  MP:" + std::to_string(player->CurrentStatistics().MagicPoints);
            spriteBatch.DrawString(Fonts::PlayerStatisticsFont(), line,
                                    Microsoft::Xna::Framework::Vector2(x, y + 25.0f), Fonts::DisplayColor);
            x += 260.0f;
        }
        if (!actionText_.empty()) {
            spriteBatch.DrawString(Fonts::HeaderFont(), actionText_, Microsoft::Xna::Framework::Vector2(10.0f, y - 30.0f),
                                    Fonts::CaptionColor);
        }
        spriteBatch.End();
    }

private:
    ScreenManager* screenManager_;
    std::string actionText_;
};

} // namespace RolePlaying
