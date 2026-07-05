#pragma once

// GearDrop.hpp -- C++ port of RolePlayingGameData/Gear/GearDrop.cs.

#include <algorithm>
#include <string>

namespace RolePlayingGameData {

// Description of how often a particular gear drops, typically from a Monster.
class GearDrop {
public:
    std::string GearName;

    int DropPercentage() const { return dropPercentage_; }
    void SetDropPercentage(int v) { dropPercentage_ = std::clamp(v, 0, 100); }

private:
    int dropPercentage_ = 0;
};

} // namespace RolePlayingGameData
