#pragma once

// CombatEngine.hpp -- simplified adaptation of Combat/CombatEngine.cs (plus
// Combatant/CombatantPlayer/CombatantMonster/ArtificialIntelligence and the
// Combat/Actions/* classes). The original is a ~2900-line real-time combat
// stage: combatants have on-screen positions, walk/attack/dodge/hit/die
// animation states, floating damage-number effects, projectile-travel timing
// for melee/spell/item actions, and a full turn-order/AI state machine.
//
// This port keeps the same *outcome* (turn-based combat against the same
// FixedCombat/RandomCombat monster data, StatisticsValue-based damage math,
// gold/experience/gear-drop rewards, and win/lose/flee results) but drives it
// through a plain text action menu (Attack/Defend/Flee, cycling through
// living party members in order) instead of the original's animated battle
// stage and its Melee/Spell/Item action-class hierarchy -- porting the full
// animation/effect state machine and per-action classes was not a productive
// use of remaining scope for this session. See missing.md's "Combat
// simplified to a text-menu turn resolver" section.
//
// StartNewCombat/CheckForEndOfCombat/GrantRewardsAndEnd are declared here but
// defined in RolePlayingGame.hpp's cross-referencing assembly section, since
// they need Session (for the party, screen manager, and reward/game-over
// screens) which itself needs CombatEngine::StartNewCombat -- a genuine
// mutual dependency, resolved the same way this project resolves any other
// forward-reference cycle between classes (see NinjAcademy/CardsStarterKit
// precedent).

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "System/Random.hpp"

#include "../AudioManager.hpp"
#include "../Data/Characters/Monster.hpp"
#include "../Data/Characters/Player.hpp"
#include "../Data/Map/FixedCombat.hpp"
#include "../Data/Map/RandomCombat.hpp"
#include "../Data/MapEntry.hpp"
#include "../Fonts.hpp"
#include "../InputManager.hpp"
#include "../ScreenManager/ScreenManager.hpp"

namespace RolePlaying {

using RolePlayingGameData::FixedCombat;
using RolePlayingGameData::MapEntry;
using RolePlayingGameData::Monster;
using RolePlayingGameData::Player;
using RolePlayingGameData::RandomCombat;

class Session; // fwd decl -- rewards/game-over routed through Session

class CombatEngine {
public:
    struct CombatMonster {
        std::shared_ptr<Monster> monster;
        int currentHealthPoints = 0;
        bool IsDead() const { return currentHealthPoints <= 0; }
    };

    enum class Phase { SelectAction, MonstersActing, Victory, Defeat, Fled };

    static bool IsActive() { return active_; }

    // Defined in RolePlayingGame.hpp -- needs Session::GetParty().
    static void StartNewCombat(const std::shared_ptr<MapEntry<FixedCombat>>& fixedCombatEntry);
    static void StartNewCombat(const std::shared_ptr<RandomCombat>& randomCombat);

    static void ClearCombat() {
        active_ = false;
        monsters_.clear();
        combatPlayers_.clear();
        currentPlayerIndex_ = 0;
        selectedMonster_ = 0;
        logLines_.clear();
    }

    static void Update(const Microsoft::Xna::Framework::GameTime& gameTime) {
        (void)gameTime;
        if (!active_) return;

        switch (phase_) {
            case Phase::SelectAction:
                HandlePlayerActionInput();
                break;
            case Phase::MonstersActing:
                ResolveMonsterTurn();
                CheckForEndOfCombat();
                break;
            case Phase::Victory:
            case Phase::Defeat:
            case Phase::Fled:
                // Terminal states are resolved synchronously in
                // CheckForEndOfCombat()/AttemptFlee(); Update() should not
                // observe them for more than one frame.
                break;
        }
    }

    static void Draw(const Microsoft::Xna::Framework::GameTime& gameTime) {
        (void)gameTime;
        if (!active_ || combatPlayers_.empty()) return;

        auto& spriteBatch = screenManager_->getSpriteBatch();
        auto& viewport = screenManager_->getGraphicsDeviceProperty().getViewportProperty();
        float x = 20.0f;
        float y = 20.0f;

        spriteBatch.Begin();
        for (auto& m : monsters_) {
            std::string status = m.monster->Name() + "  HP:" + std::to_string(std::max(0, m.currentHealthPoints)) +
                                  "/" + std::to_string(m.monster->CharacterStatistics().HealthPoints);
            spriteBatch.DrawString(Fonts::HeaderFont(), status, Microsoft::Xna::Framework::Vector2(x, y),
                                   m.IsDead() ? Fonts::RestrictionColor : Fonts::TitleColor);
            y += 36.0f;
        }

        y += 20.0f;
        if (phase_ == Phase::SelectAction && currentPlayerIndex_ < (int)combatPlayers_.size()) {
            auto& player = combatPlayers_[currentPlayerIndex_];
            std::string prompt = player->Name() + "'s turn -- Attack: " +
                                  monsters_[selectedMonster_].monster->Name() + " (Up/Down to change target)";
            spriteBatch.DrawString(Fonts::DescriptionFont(), prompt, Microsoft::Xna::Framework::Vector2(x, y),
                                   Fonts::CaptionColor);
            y += 30.0f;
            spriteBatch.DrawString(Fonts::ButtonNamesFont(), "Ok: Attack   Space: Defend   Escape: Flee",
                                   Microsoft::Xna::Framework::Vector2(x, y), Fonts::HighlightColor);
        }

        y += 40.0f;
        for (auto it = logLines_.rbegin(); it != logLines_.rend() && y < viewport.getHeightProperty() - 100; ++it) {
            spriteBatch.DrawString(Fonts::DescriptionFont(), *it, Microsoft::Xna::Framework::Vector2(x, y),
                                   Microsoft::Xna::Framework::Color(255, 255, 255, 255));
            y += 24.0f;
        }
        spriteBatch.End();
    }

    static void SetScreenManager(ScreenManager& sm) { screenManager_ = &sm; }

private:
    static System::Random& Random() {
        static System::Random random;
        return random;
    }

    static void Log(const std::string& line) {
        logLines_.push_back(line);
        if (logLines_.size() > 6) logLines_.erase(logLines_.begin());
    }

    static void BeginEncounter() {
        active_ = true;
        phase_ = Phase::SelectAction;
        currentPlayerIndex_ = 0;
        selectedMonster_ = 0;
        logLines_.clear();
        AudioManager::PushMusic("BattleTheme");
        AdvanceToNextLivingPlayer(true);
    }

    static void AdvanceToNextLivingPlayer(bool fromStart = false) {
        int start = fromStart ? 0 : currentPlayerIndex_ + 1;
        for (int i = start; i < (int)combatPlayers_.size(); i++) {
            if (combatPlayers_[i]->CurrentStatistics().HealthPoints > 0) {
                currentPlayerIndex_ = i;
                phase_ = Phase::SelectAction;
                return;
            }
        }
        // every remaining player has acted -- monsters go next
        phase_ = Phase::MonstersActing;
    }

    static void HandlePlayerActionInput() {
        if (combatPlayers_.empty() || currentPlayerIndex_ >= (int)combatPlayers_.size()) return;

        if (InputManager::IsActionTriggered(InputManager::Action::TargetUp)) {
            do {
                selectedMonster_ = (selectedMonster_ + monsters_.size() - 1) % monsters_.size();
            } while (monsters_[selectedMonster_].IsDead());
        }
        if (InputManager::IsActionTriggered(InputManager::Action::TargetDown)) {
            do {
                selectedMonster_ = (selectedMonster_ + 1) % monsters_.size();
            } while (monsters_[selectedMonster_].IsDead());
        }

        auto& attacker = *combatPlayers_[currentPlayerIndex_];

        if (InputManager::IsActionTriggered(InputManager::Action::Ok)) {
            ApplyDamage(attacker, monsters_[selectedMonster_]);
            AdvanceToNextLivingPlayer();
        } else if (InputManager::IsActionTriggered(InputManager::Action::CharacterManagement)) {
            Log(attacker.Name() + " defends.");
            AdvanceToNextLivingPlayer();
        } else if (InputManager::IsActionTriggered(InputManager::Action::Back) ||
                   InputManager::IsActionTriggered(InputManager::Action::ExitGame)) {
            AttemptFlee();
        }
    }

    static void AttemptFlee() {
        if ((int)Random().Next(0, 100) < fleeProbability_) {
            Log("The party flees!");
            phase_ = Phase::Fled;
        } else {
            Log("Couldn't escape!");
            phase_ = Phase::MonstersActing;
        }
    }

    static void ApplyDamage(Player& attacker, CombatMonster& target) {
        if (target.IsDead()) return;
        auto weapon = attacker.GetEquippedWeapon();
        int damage = weapon ? weapon->TargetDamageRange.GenerateValue(Random())
                             : attacker.CharacterStatistics().PhysicalOffense;
        damage = std::max(1, damage - target.monster->CharacterStatistics().PhysicalDefense / 2);
        target.currentHealthPoints -= damage;
        AudioManager::PlayCue(weapon ? weapon->HitCueName : std::string("SwordHit"));
        Log(attacker.Name() + " hits " + target.monster->Name() + " for " + std::to_string(damage) + ".");
        if (target.IsDead()) Log(target.monster->Name() + " is defeated!");
    }

    static void MonsterAttack(CombatMonster& attacker) {
        std::vector<int> livingIndices;
        for (int i = 0; i < (int)combatPlayers_.size(); i++)
            if (combatPlayers_[i]->CurrentStatistics().HealthPoints > 0) livingIndices.push_back(i);
        if (livingIndices.empty()) return;

        int targetIndex = livingIndices[Random().Next(0, (int)livingIndices.size())];
        auto& target = *combatPlayers_[targetIndex];

        if ((int)Random().Next(0, 100) < attacker.monster->DefendPercentage) {
            Log(attacker.monster->Name() + "'s attack is blocked by " + target.Name() + ".");
            return;
        }

        int damage = attacker.monster->CharacterStatistics().PhysicalOffense -
                     target.CharacterStatistics().PhysicalDefense / 2;
        damage = std::max(1, damage);
        target.StatisticsModifiers.HealthPoints -= damage;
        Log(attacker.monster->Name() + " hits " + target.Name() + " for " + std::to_string(damage) + ".");
    }

    static void ResolveMonsterTurn() {
        for (auto& m : monsters_) {
            if (!m.IsDead()) MonsterAttack(m);
        }
    }

    // Defined in RolePlayingGame.hpp -- needs Session for rewards/game-over.
    static void CheckForEndOfCombat();
    static void GrantRewardsAndEnd();

    static inline bool active_ = false;
    static inline Phase phase_ = Phase::SelectAction;
    static inline std::vector<CombatMonster> monsters_;
    static inline std::vector<std::shared_ptr<Player>> combatPlayers_;
    static inline int currentPlayerIndex_ = 0;
    static inline int selectedMonster_ = 0;
    static inline int fleeProbability_ = 50;
    static inline std::vector<std::string> logLines_;
    static inline std::shared_ptr<MapEntry<FixedCombat>> activeFixedCombatEntry_;
    static inline ScreenManager* screenManager_ = nullptr;
};

} // namespace RolePlaying
