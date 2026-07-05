#pragma once

// Animation.hpp -- C++ port of RolePlayingGameData/Animation/Animation.cs.

#include <string>

#include "ContentObject.hpp"

namespace RolePlayingGameData {

// An animation description for an AnimatingSprite object.
class Animation : public ContentObject {
public:
    std::string Name;
    int StartingFrame = 0;
    int EndingFrame = 0;
    int Interval = 0;
    bool IsLoop = false;

    Animation() = default;
    Animation(std::string name, int startingFrame, int endingFrame, int interval, bool isLoop)
        : Name(std::move(name)), StartingFrame(startingFrame), EndingFrame(endingFrame),
          Interval(interval), IsLoop(isLoop) {}
};

} // namespace RolePlayingGameData
