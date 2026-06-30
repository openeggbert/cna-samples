#pragma once

#include <map>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundEffect.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundEffectInstance.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundState.hpp"

namespace CatapultWars {

using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::Audio::SoundEffect;
using Microsoft::Xna::Framework::Audio::SoundEffectInstance;
using Microsoft::Xna::Framework::Audio::SoundState;

// Manages audio playback for all sounds. The XNA original is a GameComponent
// singleton; here it is a static utility holding a bank of SoundEffectInstances
// keyed by a friendly name.
class AudioManager {
public:
    static void Initialize(Game* game) {
        game_ = game;
    }

    // Loads the sounds and organizes them for future usage.
    static void LoadSounds() {
        // { content asset under "Sounds/", friendly name used by the game }
        static const std::pair<const char*, const char*> soundNames[] = {
            {"CatapultExplosion", "catapultExplosion"},
            {"Lose",              "gameOver_Lose"},
            {"Win",               "gameOver_Win"},
            {"BoulderHit",        "boulderHit"},
            {"CatapultFire",      "catapultFire"},
            {"RopeStretch",       "ropeStretch"},
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

    // Pause or resume all sounds, to support the pause screen. (Mirrors the
    // XNA original's behaviour verbatim.)
    static void PauseResumeSounds(bool isPause) {
        SoundState state = isPause ? SoundState::Paused : SoundState::Playing;
        for (auto& kv : soundBank_) {
            if (kv.second.getStateProperty() == state) {
                if (isPause)
                    kv.second.Play();
                else
                    kv.second.Pause();
            }
        }
    }

    static void PlayMusic(const std::string& musicSoundName) {
        if (musicSound_ != nullptr)
            musicSound_->Stop(true);

        auto it = soundBank_.find(musicSoundName);
        if (it != soundBank_.end()) {
            musicSound_ = &it->second;
            if (!musicSound_->getIsLoopedProperty())
                musicSound_->setIsLoopedProperty(true);
            musicSound_->Play();
        }
    }

private:
    inline static Game* game_ = nullptr;
    inline static std::vector<SoundEffect> soundEffects_;
    inline static std::map<std::string, SoundEffectInstance> soundBank_;
    inline static SoundEffectInstance* musicSound_ = nullptr;
};

} // namespace CatapultWars
