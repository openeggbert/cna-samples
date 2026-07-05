#pragma once

// InnScreen.hpp -- simplified adaptation of GameScreens/InnScreen.cs. Charges
// ChargePerPlayer gold per party member and fully restores HP/MP (zeroes out
// each player's negative StatisticsModifiers) if the party can afford it.

#include <memory>

#include "../AudioManager.hpp"
#include "../Data/Map/Inn.hpp"
#include "../Fonts.hpp"
#include "../ScreenManager/MenuEntry.hpp"
#include "../ScreenManager/MenuScreen.hpp"
#include "../Session/Session.hpp"

namespace RolePlaying {

class InnScreen : public MenuScreen {
public:
    explicit InnScreen(const std::shared_ptr<RolePlayingGameData::Inn>& inn) : inn_(inn) {
        SetIsPopup(true);
        auto rest = std::make_shared<MenuEntry>("Rest");
        rest->Selected = [this]() { Rest(); };
        auto leave = std::make_shared<MenuEntry>("Leave");
        leave->Selected = [this]() { ExitScreen(); };
        MenuEntries().push_back(rest);
        MenuEntries().push_back(leave);
    }

    void LoadContent() override {
        auto& viewport = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty();
        float x = (float)(viewport.getWidthProperty() / 2 - 80);
        float y = (float)(viewport.getHeightProperty() / 2 - 20);
        for (auto& entry : MenuEntries()) {
            entry->Font = &Fonts::HeaderFont();
            entry->Position = Microsoft::Xna::Framework::Vector2(x, y);
            y += 50.0f;
        }
    }

private:
    void Rest() {
        auto* party = Session::GetParty();
        if (!party) { ExitScreen(); return; }
        int cost = inn_->ChargePerPlayer * (int)party->Players.size();
        if (party->PartyGold() >= cost) {
            party->AddPartyGold(-cost);
            for (auto& player : party->Players) {
                player->StatisticsModifiers = RolePlayingGameData::StatisticsValue();
            }
            AudioManager::PlayCue("Continue");
        }
        ExitScreen();
    }

    std::shared_ptr<RolePlayingGameData::Inn> inn_;
};

} // namespace RolePlaying
