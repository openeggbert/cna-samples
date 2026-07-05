#pragma once

// GameRule.hpp -- C++ port of Rules/GameRule.cs (XNA 4.0 CardsStarterKit sample).

#include "System/EventArgs.hpp"
#include "System/EventHandler.hpp"

namespace CardsFramework {

// Represents a rule in a card game. Inherit and implement Check().
class GameRule {
public:
    // An event which triggers when the rule conditions are matched.
    System::EventHandler<System::EventArgs> RuleMatch;

    virtual ~GameRule() = default;

    // Checks whether the rule conditions are met. Should call FireRuleMatch().
    virtual void Check() = 0;

protected:
    void FireRuleMatch(const System::EventArgs& e) { RuleMatch.Raise(nullptr, e); }
};

} // namespace CardsFramework
