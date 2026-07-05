#pragma once

// WorldObject.hpp -- C++ port of RolePlayingGameData/WorldObject.cs.

#include <string>

#include "ContentObject.hpp"

namespace RolePlayingGameData {

// Common base class for all objects that are visible in the world.
class WorldObject : public ContentObject {
public:
    const std::string& Name() const { return name_; }
    void SetName(const std::string& v) { name_ = v; }

private:
    std::string name_;
};

} // namespace RolePlayingGameData
