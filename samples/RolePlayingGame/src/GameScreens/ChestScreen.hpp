#pragma once

// ChestScreen.hpp -- simplified adaptation of GameScreens/ChestScreen.cs. The
// original shows a scrollable icon grid of the chest's contents with
// per-item Take/TakeAll. This port offers a single "Take All" action (adds
// every item + gold to the party inventory, then empties/removes the chest)
// since per-item selection is a presentation-layer nicety, not a gameplay
// mechanic this port needs to preserve. See missing.md.

#include <memory>

#include "../AudioManager.hpp"
#include "../Data/Map/Chest.hpp"
#include "../Data/MapEntry.hpp"
#include "../Fonts.hpp"
#include "../ScreenManager/MenuEntry.hpp"
#include "../ScreenManager/MenuScreen.hpp"
#include "../Session/Session.hpp"

namespace RolePlaying {

class ChestScreen : public MenuScreen {
public:
    explicit ChestScreen(const std::shared_ptr<RolePlayingGameData::MapEntry<RolePlayingGameData::Chest>>& chestEntry)
        : chestEntry_(chestEntry) {
        SetIsPopup(true);
        auto takeAll = std::make_shared<MenuEntry>("Take All");
        takeAll->Selected = [this]() { TakeAll(); };
        auto leave = std::make_shared<MenuEntry>("Leave");
        leave->Selected = [this]() { ExitScreen(); };
        MenuEntries().push_back(takeAll);
        MenuEntries().push_back(leave);
    }

    void LoadContent() override {
        for (auto& entry : MenuEntries()) entry->Font = &Fonts::HeaderFont();
        auto& viewport = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty();
        float x = (float)(viewport.getWidthProperty() / 2 - 100);
        float y = (float)(viewport.getHeightProperty() / 2 - 20);
        for (auto& entry : MenuEntries()) {
            entry->Position = Microsoft::Xna::Framework::Vector2(x, y);
            y += 50.0f;
        }
    }

private:
    void TakeAll() {
        auto* party = Session::GetParty();
        auto& chest = *chestEntry_->Content;
        if (party) {
            party->AddPartyGold(chest.Gold);
            for (auto& entry : chest.Entries) party->AddToInventory(entry->Content, entry->Count);
        }
        chest.Gold = 0;
        chest.Entries.clear();
        AudioManager::PlayCue("Money");
        Session::StoreModifiedChest(chestEntry_);
        if (chest.IsEmpty()) Session::RemoveChest(chestEntry_);
        ExitScreen();
    }

    std::shared_ptr<RolePlayingGameData::MapEntry<RolePlayingGameData::Chest>> chestEntry_;
};

} // namespace RolePlaying
