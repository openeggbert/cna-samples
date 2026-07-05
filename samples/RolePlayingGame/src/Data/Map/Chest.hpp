#pragma once

// Chest.hpp -- C++ port of RolePlayingGameData/Map/Chest.cs.

#include <memory>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include "../ContentEntry.hpp"
#include "../Gear/Gear.hpp"
#include "../WorldObject.hpp"

namespace RolePlayingGameData {

// A treasure chest in the game world.
class Chest : public WorldObject {
public:
    int Gold = 0;
    std::vector<std::shared_ptr<ContentEntry<Gear>>> Entries;

    bool IsEmpty() const { return Gold <= 0 && Entries.empty(); }

    std::string TextureName;
    std::shared_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> Texture;

    // The game has to handle chests that have had some contents removed
    // without modifying the original chest (and all chests that come after).
    std::shared_ptr<Chest> Clone() const {
        auto chest = std::make_shared<Chest>();
        chest->Gold = Gold;
        chest->SetName(Name());
        chest->Texture = Texture;
        chest->TextureName = TextureName;
        for (auto& originalEntry : Entries) {
            auto newEntry = std::make_shared<ContentEntry<Gear>>();
            newEntry->Count = originalEntry->Count;
            newEntry->ContentName = originalEntry->ContentName;
            newEntry->Content = originalEntry->Content;
            chest->Entries.push_back(newEntry);
        }
        return chest;
    }
};

} // namespace RolePlayingGameData
