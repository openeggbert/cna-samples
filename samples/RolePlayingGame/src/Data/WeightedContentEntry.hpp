#pragma once

// WeightedContentEntry.hpp -- C++ port of RolePlayingGameData/WeightedContentEntry.cs.

#include "ContentEntry.hpp"

namespace RolePlayingGameData {

// A description of a piece of content, quantity and weight for various purposes.
template <typename T>
class WeightedContentEntry : public ContentEntry<T> {
public:
    int Weight = 0;
};

} // namespace RolePlayingGameData
