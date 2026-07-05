#pragma once

// Inn.hpp -- C++ port of RolePlayingGameData/Map/Inn.cs.

#include <memory>
#include <string>

#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include "../WorldObject.hpp"

namespace RolePlayingGameData {

// An inn in the game world, where the party can rest and restore themselves.
class Inn : public WorldObject {
public:
    int ChargePerPlayer = 0;

    std::string WelcomeMessage;
    std::string PaidMessage;
    std::string NotEnoughGoldMessage;

    std::string ShopkeeperTextureName;
    std::shared_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> ShopkeeperTexture;
};

} // namespace RolePlayingGameData
