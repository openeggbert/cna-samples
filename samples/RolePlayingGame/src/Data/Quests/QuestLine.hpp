#pragma once

// QuestLine.hpp -- C++ port of RolePlayingGameData/Quests/QuestLine.cs.

#include <memory>
#include <string>
#include <vector>

#include "../ContentObject.hpp"
#include "Quest.hpp"

namespace RolePlayingGameData {

// A line of quests, presented to the player in order -- only one quest is
// presented at a time and must be completed before the line can continue.
class QuestLine : public ContentObject {
public:
    std::string Name;
    std::vector<std::string> QuestContentNames;
    std::vector<std::shared_ptr<Quest>> Quests;

    std::shared_ptr<QuestLine> Clone() const {
        auto line = std::make_shared<QuestLine>();
        line->SetAssetName(AssetName());
        line->Name = Name;
        line->QuestContentNames = QuestContentNames;
        for (auto& q : Quests) line->Quests.push_back(q->Clone());
        return line;
    }
};

} // namespace RolePlayingGameData
