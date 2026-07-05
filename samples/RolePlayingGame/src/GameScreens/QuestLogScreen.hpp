#pragma once

// QuestLogScreen.hpp -- simplified adaptation of GameScreens/QuestLogScreen.cs
// -- shows the quest's name/objective (or destination-objective, once
// requirements are met) as a dismissable DialogueScreen.

#include <memory>

#include "../Data/Quests/Quest.hpp"
#include "DialogueScreen.hpp"

namespace RolePlaying {

class QuestLogScreen : public DialogueScreen {
public:
    explicit QuestLogScreen(const std::shared_ptr<RolePlayingGameData::Quest>& quest) {
        TitleText = quest->Name;
        DialogueText = (quest->Stage == RolePlayingGameData::Quest::QuestStage::RequirementsMet)
                           ? quest->DestinationObjectiveMessage
                           : quest->ObjectiveMessage;
        BackText.clear();
    }
};

} // namespace RolePlaying
