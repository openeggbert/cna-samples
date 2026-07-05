#pragma once

// Portal.hpp -- C++ port of RolePlayingGameData/Map/Portal.cs.

#include <string>

#include "Microsoft/Xna/Framework/Point.hpp"

#include "../ContentObject.hpp"

namespace RolePlayingGameData {

// A transition point from one map to another.
class Portal : public ContentObject {
public:
    std::string Name;

    // The map coordinate that the party will automatically walk to after spawning on this portal.
    Microsoft::Xna::Framework::Point LandingMapPosition;

    std::string DestinationMapContentName;
    std::string DestinationMapPortalName;
};

} // namespace RolePlayingGameData
