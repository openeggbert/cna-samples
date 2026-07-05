#pragma once

// QuestRequirement.hpp -- C++ port of RolePlayingGameData/Quests/QuestRequirement.cs.

#include "../ContentEntry.hpp"

namespace RolePlayingGameData {

// A requirement for a particular number of a piece of content.
// Used to track gear acquired and monsters killed.
template <typename T>
class QuestRequirement : public ContentEntry<T> {
public:
    int CompletedCount = 0;
};

} // namespace RolePlayingGameData
