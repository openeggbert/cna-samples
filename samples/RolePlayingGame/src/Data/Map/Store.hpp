#pragma once

// Store.hpp -- C++ port of RolePlayingGameData/Map/Store.cs.

#include <memory>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include "../WorldObject.hpp"
#include "StoreCategory.hpp"

namespace RolePlayingGameData {

// A gear store, where the party can buy and sell gear, organized into categories.
class Store : public WorldObject {
public:
    float BuyMultiplier = 1.0f;
    float SellMultiplier = 1.0f;
    std::vector<StoreCategory> StoreCategories;

    std::string WelcomeMessage;

    std::string ShopkeeperTextureName;
    std::shared_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> ShopkeeperTexture;
};

} // namespace RolePlayingGameData
