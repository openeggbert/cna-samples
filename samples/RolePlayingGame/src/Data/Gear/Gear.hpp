#pragma once

// Gear.hpp -- C++ port of RolePlayingGameData/Gear/Gear.cs.

#include <memory>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"

#include "../ContentObject.hpp"

namespace RolePlayingGameData {

class FightingCharacter;

// An inventory element -- items, equipment, etc.
class Gear : public ContentObject {
public:
    ~Gear() override = default;

    std::string Name;
    std::string Description;

    virtual std::string GetPowerText() const { return ""; }

    // If the value is less than zero, it cannot be sold.
    int GoldValue = 0;
    // If true, the gear can be dropped. If false, it cannot ever be dropped.
    bool IsDroppable = false;

    int MinimumCharacterLevel = 0;
    // Class names are compared case-insensitive.
    std::vector<std::string> SupportedClasses;

    virtual bool CheckRestrictions(const FightingCharacter& fightingCharacter) const;

    virtual std::string GetRestrictionsText() const {
        std::string sb;
        if (MinimumCharacterLevel > 0) {
            sb += "Level - " + std::to_string(MinimumCharacterLevel) + "; ";
        }
        if (!SupportedClasses.empty()) {
            sb += "Class - ";
            bool first = true;
            for (auto& className : SupportedClasses) {
                if (first) first = false; else sb += ",";
                sb += className;
            }
        }
        return sb;
    }

    std::string IconTextureName;
    std::shared_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> IconTexture;

    virtual void DrawIcon(Microsoft::Xna::Framework::Graphics::SpriteBatch& spriteBatch,
                          Microsoft::Xna::Framework::Vector2 position) const {
        if (IconTexture) {
            spriteBatch.Draw(*IconTexture, position, Microsoft::Xna::Framework::Color(255, 255, 255, 255));
        }
    }

    virtual void DrawDescription(Microsoft::Xna::Framework::Graphics::SpriteBatch& spriteBatch,
                                 Microsoft::Xna::Framework::Graphics::SpriteFont& spriteFont,
                                 Microsoft::Xna::Framework::Color color,
                                 Microsoft::Xna::Framework::Vector2 position,
                                 int maximumCharactersPerLine, int maximumLines) const {
        if (Description.empty()) return;
        spriteBatch.DrawString(spriteFont, Description, position, color);
    }
};

} // namespace RolePlayingGameData
