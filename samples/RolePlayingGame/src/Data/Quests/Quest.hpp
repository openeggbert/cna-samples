#pragma once

// Quest.hpp -- C++ port of RolePlayingGameData/Quests/Quest.cs.

#include <memory>
#include <string>
#include <vector>

#include "../Characters/Monster.hpp"
#include "../ContentObject.hpp"
#include "../Gear/Gear.hpp"
#include "../Map/Chest.hpp"
#include "../Map/FixedCombat.hpp"
#include "../WorldEntry.hpp"
#include "QuestRequirement.hpp"

namespace RolePlayingGameData {

// A quest that the party can embark on, with goals and rewards.
class Quest : public ContentObject {
public:
    enum class QuestStage { NotStarted, InProgress, RequirementsMet, Completed };

    QuestStage Stage = QuestStage::NotStarted;

    std::string Name;
    std::string Description;
    std::string ObjectiveMessage;
    std::string CompletionMessage;

    std::vector<std::shared_ptr<QuestRequirement<Gear>>> GearRequirements;
    std::vector<std::shared_ptr<QuestRequirement<Monster>>> MonsterRequirements;

    bool AreRequirementsMet() const {
        for (auto& r : GearRequirements)
            if (r->CompletedCount < r->Count) return false;
        for (auto& r : MonsterRequirements)
            if (r->CompletedCount < r->Count) return false;
        return true;
    }

    std::vector<std::shared_ptr<WorldEntry<FixedCombat>>> FixedCombatEntries;
    std::vector<std::shared_ptr<WorldEntry<Chest>>> ChestEntries;

    std::string DestinationMapContentName;
    std::string DestinationNpcContentName;
    std::string DestinationObjectiveMessage;

    int ExperienceReward = 0;
    int GoldReward = 0;
    std::vector<std::string> GearRewardContentNames;
    std::vector<std::shared_ptr<Gear>> GearRewards;

    std::shared_ptr<Quest> Clone() const {
        auto quest = std::make_shared<Quest>();
        quest->SetAssetName(AssetName());
        for (auto& chestEntry : ChestEntries) {
            auto worldEntry = std::make_shared<WorldEntry<Chest>>();
            worldEntry->Content = chestEntry->Content ? chestEntry->Content->Clone() : nullptr;
            worldEntry->ContentName = chestEntry->ContentName;
            worldEntry->Count = chestEntry->Count;
            worldEntry->EntryDirection = chestEntry->EntryDirection;
            worldEntry->MapContentName = chestEntry->MapContentName;
            worldEntry->MapPosition = chestEntry->MapPosition;
            quest->ChestEntries.push_back(worldEntry);
        }
        quest->CompletionMessage = CompletionMessage;
        quest->Description = Description;
        quest->DestinationMapContentName = DestinationMapContentName;
        quest->DestinationNpcContentName = DestinationNpcContentName;
        quest->DestinationObjectiveMessage = DestinationObjectiveMessage;
        quest->ExperienceReward = ExperienceReward;
        quest->FixedCombatEntries = FixedCombatEntries;
        quest->GearRequirements = GearRequirements;
        quest->GearRewardContentNames = GearRewardContentNames;
        quest->GearRewards = GearRewards;
        quest->GoldReward = GoldReward;
        quest->MonsterRequirements = MonsterRequirements;
        quest->Name = Name;
        quest->ObjectiveMessage = ObjectiveMessage;
        quest->Stage = Stage;
        return quest;
    }
};

} // namespace RolePlayingGameData
