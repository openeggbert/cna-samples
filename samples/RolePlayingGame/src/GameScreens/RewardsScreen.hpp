#pragma once

// RewardsScreen.hpp -- simplified adaptation of GameScreens/RewardsScreen.cs
// -- reports gold/experience/gear rewards as a dismissable DialogueScreen and
// applies them to the party immediately (the original applies them via a
// TakeGear-icon-grid; this port applies everything unconditionally).

#include <memory>
#include <string>
#include <vector>

#include "../Data/Gear/Gear.hpp"
#include "../Session/Session.hpp"
#include "DialogueScreen.hpp"

namespace RolePlaying {

class RewardsScreen : public DialogueScreen {
public:
    enum class RewardScreenMode { Combat, Quest };

    RewardsScreen(RewardScreenMode mode, int experience, int gold,
                  const std::vector<std::shared_ptr<RolePlayingGameData::Gear>>& gear) {
        auto* party = Session::GetParty();
        if (party) {
            party->GiveExperience(experience);
            party->AddPartyGold(gold);
            for (auto& g : gear) party->AddToInventory(g, 1);
        }

        TitleText = (mode == RewardScreenMode::Quest) ? "Quest Reward" : "Victory!";
        std::string text = "Experience: " + std::to_string(experience) + "\nGold: " + std::to_string(gold);
        if (!gear.empty()) {
            text += "\nItems:";
            for (auto& g : gear) text += " " + g->Name + ";";
        }
        DialogueText = text;
        BackText.clear();
    }
};

} // namespace RolePlaying
