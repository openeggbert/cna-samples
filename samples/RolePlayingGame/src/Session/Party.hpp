#pragma once

// Party.hpp -- C++ port of Session/Party.cs. The group of players, under
// control of the user.

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"

#include "../Data/Characters/Monster.hpp"
#include "../Data/Characters/Player.hpp"
#include "../Data/ContentEntry.hpp"
#include "../Data/ContentLoader.hpp"
#include "../Data/GameStartDescription.hpp"

namespace RolePlaying {

using RolePlayingGameData::ContentEntry;
using RolePlayingGameData::ContentLoader;
using RolePlayingGameData::Gear;
using RolePlayingGameData::GameStartDescription;
using RolePlayingGameData::Monster;
using RolePlayingGameData::Player;

class Party {
public:
    // The first entry is the leader.
    std::vector<std::shared_ptr<Player>> Players;

    void JoinParty(const std::shared_ptr<Player>& player) {
        Players.push_back(player);
        partyGold_ += player->Gold;
        player->Gold = 0;
        for (auto& entry : player->Inventory) AddToInventory(entry->Content, entry->Count);
        player->Inventory.clear();
    }

    // Defined out-of-line where Session/LevelUpScreen are available (see
    // GameplayScreen.hpp's cross-referencing definitions).
    void GiveExperience(int experience);

    const std::vector<std::shared_ptr<ContentEntry<Gear>>>& Inventory() const { return inventory_; }

    void AddToInventory(const std::shared_ptr<Gear>& gear, int count) {
        if (!gear || count <= 0) return;
        for (auto& entry : inventory_) {
            if (entry->Content == gear) { entry->Count += count; return; }
        }
        auto entry = std::make_shared<ContentEntry<Gear>>();
        entry->Content = gear;
        entry->Count = count;
        entry->ContentName = gear->AssetName();
        const std::string prefix = "Gear/";
        if (entry->ContentName.rfind(prefix, 0) == 0) entry->ContentName = entry->ContentName.substr(prefix.size());
        inventory_.push_back(entry);
    }

    bool RemoveFromInventory(const std::shared_ptr<Gear>& gear, int count) {
        for (size_t i = 0; i < inventory_.size(); i++) {
            if (inventory_[i]->Content == gear) {
                inventory_[i]->Count -= count;
                bool fullRemoval = inventory_[i]->Count >= 0;
                if (inventory_[i]->Count <= 0) inventory_.erase(inventory_.begin() + i);
                return fullRemoval;
            }
        }
        return false;
    }

    int PartyGold() const { return partyGold_; }
    void SetPartyGold(int v) { partyGold_ = v; }
    void AddPartyGold(int delta) { partyGold_ += delta; }

    const std::unordered_map<std::string, int>& MonsterKills() const { return monsterKills_; }
    void AddMonsterKill(const Monster& monster) { monsterKills_[monster.AssetName()]++; }
    void ClearMonsterKills() { monsterKills_.clear(); }

    Party() = default;

    Party(const GameStartDescription& gameStartDescription, ContentLoader& loader) {
        for (auto& name : gameStartDescription.PlayerContentNames) {
            JoinParty(loader.LoadPlayer(name)->Clone());
        }
    }

private:
    std::vector<std::shared_ptr<ContentEntry<Gear>>> inventory_;
    int partyGold_ = 0;
    std::unordered_map<std::string, int> monsterKills_;
};

} // namespace RolePlaying
