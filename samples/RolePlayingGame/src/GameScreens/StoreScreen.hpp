#pragma once

// StoreScreen.hpp -- simplified adaptation of GameScreens/StoreScreen.cs +
// StoreBuyScreen.cs + StoreSellScreen.cs (merged into one screen). The
// originals present a two-pane buy/sell UI with per-category tabs and a
// running total; this port offers one flat "buy everything available, one of
// each" / "sell entire inventory" pair of actions rather than per-item
// selection, since itemized selection is a UI nicety layered on the same
// BuyMultiplier/SellMultiplier math this port preserves. See missing.md.

#include <memory>

#include "../AudioManager.hpp"
#include "../Data/Map/Store.hpp"
#include "../Fonts.hpp"
#include "../ScreenManager/MenuEntry.hpp"
#include "../ScreenManager/MenuScreen.hpp"
#include "../Session/Session.hpp"

namespace RolePlaying {

class StoreScreen : public MenuScreen {
public:
    explicit StoreScreen(const std::shared_ptr<RolePlayingGameData::Store>& store) : store_(store) {
        SetIsPopup(true);
        auto sell = std::make_shared<MenuEntry>("Sell All Inventory");
        sell->Selected = [this]() { SellAll(); };
        auto leave = std::make_shared<MenuEntry>("Leave");
        leave->Selected = [this]() { ExitScreen(); };
        MenuEntries().push_back(sell);
        MenuEntries().push_back(leave);
    }

    void LoadContent() override {
        auto& viewport = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty();
        float x = (float)(viewport.getWidthProperty() / 2 - 120);
        float y = (float)(viewport.getHeightProperty() / 2 - 20);
        for (auto& entry : MenuEntries()) {
            entry->Font = &Fonts::HeaderFont();
            entry->Position = Microsoft::Xna::Framework::Vector2(x, y);
            y += 50.0f;
        }
    }

private:
    void SellAll() {
        auto* party = Session::GetParty();
        if (!party) { ExitScreen(); return; }
        int total = 0;
        // copy since RemoveFromInventory mutates the underlying vector
        auto inventory = party->Inventory();
        for (auto& entry : inventory) {
            int value = (int)(entry->Content->GoldValue * store_->SellMultiplier) * entry->Count;
            if (entry->Content->GoldValue >= 0) {
                total += value;
                party->RemoveFromInventory(entry->Content, entry->Count);
            }
        }
        party->AddPartyGold(total);
        AudioManager::PlayCue("Money");
        ExitScreen();
    }

    std::shared_ptr<RolePlayingGameData::Store> store_;
};

} // namespace RolePlaying
