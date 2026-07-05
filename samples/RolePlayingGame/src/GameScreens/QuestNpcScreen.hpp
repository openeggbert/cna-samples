#pragma once

// QuestNpcScreen.hpp -- simplified adaptation of GameScreens/QuestNpcScreen.cs
// -- shows the NPC's introduction dialogue as a dismissable DialogueScreen.

#include <memory>

#include "../Data/Characters/QuestNpc.hpp"
#include "../Data/MapEntry.hpp"
#include "DialogueScreen.hpp"

namespace RolePlaying {

class QuestNpcScreen : public DialogueScreen {
public:
    explicit QuestNpcScreen(const std::shared_ptr<RolePlayingGameData::MapEntry<RolePlayingGameData::QuestNpc>>& questNpcEntry) {
        TitleText = questNpcEntry->Content->Name();
        DialogueText = questNpcEntry->Content->IntroductionDialogue;
        BackText.clear();
    }
};

} // namespace RolePlaying
