#pragma once

// AudioManager.hpp — C++ port of Misc/AudioManager.cs (XNA 4.0 MarbleMaze sample).
// Static-singleton GameComponent that manages the 4 sound effects this sample uses
// (rolling/collision/pit/checkpoint) -- there is no background music track in this
// sample (unlike HoneycombRush's AudioManager, which this port's shape otherwise
// follows).

#include <string>
#include <unordered_map>

#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameComponent.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundEffect.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundEffectInstance.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundState.hpp"

namespace MarbleMazeSample {

using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GameComponent;
using Microsoft::Xna::Framework::Audio::SoundEffect;
using Microsoft::Xna::Framework::Audio::SoundEffectInstance;
using Microsoft::Xna::Framework::Audio::SoundState;

class AudioManager : public GameComponent {
public:
    static AudioManager* Instance() { return instance_; }

    static void Initialize(Game& game) {
        instance_ = new AudioManager(game);
        game.getComponentsProperty().Add(instance_);
    }

    static void LoadSounds() {
        LoadSound("MarbleRoll", "rolling");
        LoadSound("MarbleHit", "collision");
        LoadSound("MarbleFall", "pit");
        LoadSound("Checkpoint", "checkpoint");
    }

    // Returns nullptr if the named sound doesn't exist (mirrors the C# indexer's
    // "return null" branch).
    static SoundEffectInstance* Get(const std::string& soundName) {
        auto it = instance_->soundBank_.find(soundName);
        return it != instance_->soundBank_.end() ? &it->second : nullptr;
    }

    static void PlaySound(const std::string& soundName) {
        auto it = instance_->soundBank_.find(soundName);
        if (it != instance_->soundBank_.end()) it->second.Play();
    }

    static void PlaySound(const std::string& soundName, bool isLooped) {
        auto it = instance_->soundBank_.find(soundName);
        if (it != instance_->soundBank_.end()) {
            if (it->second.getIsLoopedProperty() != isLooped) it->second.setIsLoopedProperty(isLooped);
            it->second.Play();
        }
    }

    static void StopSound(const std::string& soundName) {
        auto it = instance_->soundBank_.find(soundName);
        if (it != instance_->soundBank_.end()) it->second.Stop();
    }

    static void StopSounds() {
        for (auto& [name, sound] : instance_->soundBank_) {
            if (sound.getStateProperty() != SoundState::Stopped) sound.Stop();
        }
    }

    // NOTE: matches the C# original's (confusing but deliberate) parameter
    // semantics exactly -- isPause=true resumes any currently-Paused instance
    // (Play()); isPause=false pauses any currently-Playing instance (Pause()).
    // Every call site in this sample uses it exactly this way (PauseScreen's
    // "Return"/"Restart" entries pass true to resume; GameplayScreen's
    // PauseCurrentGame()/CalibrateGame() pass false to pause) -- not a bug
    // introduced by this port.
    static void PauseResumeSounds(bool isPause) {
        SoundState state = isPause ? SoundState::Paused : SoundState::Playing;

        for (auto& [name, sound] : instance_->soundBank_) {
            if (sound.getStateProperty() == state) {
                if (isPause)
                    sound.Play();
                else
                    sound.Pause();
            }
        }
    }

private:
    explicit AudioManager(Game& game) : GameComponent(game) {}

    static void LoadSound(const std::string& contentName, const std::string& alias) {
        // SoundEffectInstance stores a raw pointer back to the SoundEffect it was
        // created from, so the SoundEffect must be cached here for as long as the
        // AudioManager lives (see HoneycombRush's AudioManager.hpp for the same note).
        auto [it, inserted] = instance_->soundEffects_.try_emplace(
            alias, instance_->getGameProperty().getContentProperty().Load<SoundEffect>(std::string("Sounds/") + contentName));

        if (instance_->soundBank_.find(alias) == instance_->soundBank_.end()) {
            instance_->soundBank_.emplace(alias, it->second.CreateInstance());
        }
    }

    static inline AudioManager* instance_ = nullptr;

    std::unordered_map<std::string, SoundEffect> soundEffects_;
    std::unordered_map<std::string, SoundEffectInstance> soundBank_;
};

} // namespace MarbleMazeSample
