#pragma once

// PlayerNpcScreen.hpp -- simplified adaptation of GameScreens/PlayerNpcScreen.cs.

#include <memory>

#include "../AudioManager.hpp"
#include "../Data/Characters/Player.hpp"
#include "../Data/MapEntry.hpp"
#include "../Fonts.hpp"
#include "../ScreenManager/MenuEntry.hpp"
#include "../ScreenManager/MenuScreen.hpp"
#include "../Session/Session.hpp"

namespace RolePlaying {

class PlayerNpcScreen : public MenuScreen {
public:
    explicit PlayerNpcScreen(const std::shared_ptr<RolePlayingGameData::MapEntry<RolePlayingGameData::Player>>& playerEntry)
        : playerEntry_(playerEntry) {
        SetIsPopup(true);
        auto join = std::make_shared<MenuEntry>("Ask to Join");
        join->Selected = [this]() { Join(); };
        auto leave = std::make_shared<MenuEntry>("Leave");
        leave->Selected = [this]() { ExitScreen(); };
        MenuEntries().push_back(join);
        MenuEntries().push_back(leave);
    }

    void LoadContent() override {
        auto& viewport = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty();
        float x = (float)(viewport.getWidthProperty() / 2 - 100);
        float y = (float)(viewport.getHeightProperty() / 2 - 20);
        for (auto& entry : MenuEntries()) {
            entry->Font = &Fonts::HeaderFont();
            entry->Position = Microsoft::Xna::Framework::Vector2(x, y);
            y += 50.0f;
        }
    }

private:
    void Join() {
        auto* party = Session::GetParty();
        if (party) {
            party->JoinParty(playerEntry_->Content);
            Session::RemovePlayerNpc(playerEntry_);
            AudioManager::PlayCue("Continue");
        }
        ExitScreen();
    }

    std::shared_ptr<RolePlayingGameData::MapEntry<RolePlayingGameData::Player>> playerEntry_;
};

} // namespace RolePlaying
