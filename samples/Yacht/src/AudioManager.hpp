#pragma once

#include <map>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundEffect.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundEffectInstance.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundState.hpp"
#include "System/Random.hpp"

namespace Yacht {

using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::Audio::SoundEffect;
using Microsoft::Xna::Framework::Audio::SoundEffectInstance;
using Microsoft::Xna::Framework::Audio::SoundState;

// Manages audio playback for all sounds. The XNA original (Misc/AudioManager.cs)
// is a GameComponent singleton; here it is a static utility holding a bank of
// SoundEffectInstances keyed by a friendly name (the same simplification
// already used by CatapultWars' AudioManager.hpp). Yacht has no music
// ("No music for this game" per the original's own LoadMusic() comment), so
// LoadMusic/PlayMusic/StopMusic are omitted entirely.
class AudioManager {
public:
    static void Initialize(Game* game) {
        game_ = game;
    }

    // Loads the sounds and organizes them for future usage.
    static void LoadSounds() {
        // { content asset under "Sounds/", friendly name used by the game }
        static const std::pair<const char*, const char*> soundNames[] = {
            {"DiceRoll 1",      "Roll1"},
            {"DiceRoll 2",      "Roll2"},
            {"DiceRoll 3",      "Roll3"},
            {"DiceRoll 4",      "Roll4"},
            {"Pencil 1",        "Pencil1"},
            {"Pencil 2",        "Pencil2"},
            {"Pencil 3",        "Pencil3"},
            {"DiceSelection 1", "DieSelect1"},
            {"DiceSelection 2", "DieSelect2"},
            {"Score Select",    "ScoreSelect"},
            {"Turn Change 1",   "TurnChange1"},
            {"Turn Change 2",   "TurnChange2"},
            {"Winner",          "Winner"},
            {"Loss",            "Loss"},
        };

        auto& content = game_->getContentProperty();
        soundEffects_.clear();
        soundBank_.clear();
        soundEffects_.reserve(sizeof(soundNames) / sizeof(soundNames[0]));

        for (const auto& entry : soundNames) {
            soundEffects_.push_back(
                content.Load<SoundEffect>(std::string("Sounds/") + entry.first));
            soundBank_.emplace(entry.second, soundEffects_.back().CreateInstance());
        }
    }

    static void PlaySound(const std::string& soundName) {
        auto it = soundBank_.find(soundName);
        if (it != soundBank_.end())
            it->second.Play();
    }

    // Plays a random sound by appending a number to the supplied sound name,
    // e.g. PlaySoundRandom("Roll", 4) plays one of "Roll1".."Roll4".
    static void PlaySoundRandom(const std::string& soundName, int maxNumber) {
        PlaySound(soundName + std::to_string(random_.Next(1, maxNumber + 1)));
    }

    static void PlaySound(const std::string& soundName, bool isLooped) {
        auto it = soundBank_.find(soundName);
        if (it != soundBank_.end()) {
            if (it->second.getIsLoopedProperty() != isLooped)
                it->second.setIsLoopedProperty(isLooped);
            it->second.Play();
        }
    }

    static void StopSound(const std::string& soundName) {
        auto it = soundBank_.find(soundName);
        if (it != soundBank_.end())
            it->second.Stop();
    }

    static void StopSounds() {
        for (auto& kv : soundBank_)
            if (kv.second.getStateProperty() != SoundState::Stopped)
                kv.second.Stop();
    }

    // Pause or resume all sounds. resumeSounds=true resumes paused sounds;
    // false pauses currently-playing sounds (matches the original's own,
    // non-inverted parameter semantics).
    static void PauseResumeSounds(bool resumeSounds) {
        SoundState state = resumeSounds ? SoundState::Paused : SoundState::Playing;
        for (auto& kv : soundBank_) {
            if (kv.second.getStateProperty() == state) {
                if (resumeSounds)
                    kv.second.Resume();
                else
                    kv.second.Pause();
            }
        }
    }

private:
    inline static Game* game_ = nullptr;
    inline static std::vector<SoundEffect> soundEffects_;
    inline static std::map<std::string, SoundEffectInstance> soundBank_;
    inline static System::Random random_;
};

} // namespace Yacht
