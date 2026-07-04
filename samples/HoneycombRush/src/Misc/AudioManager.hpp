#pragma once

// AudioManager.hpp — C++ port of Misc/AudioManager.cs (XNA 4.0 HoneycombRush
// sample). Static-singleton component that manages sound/music playback.

#include <string>
#include <unordered_map>

#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameComponent.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundEffect.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundEffectInstance.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundState.hpp"
#include "Microsoft/Xna/Framework/Media/MediaPlayer.hpp"
#include "Microsoft/Xna/Framework/Media/MediaState.hpp"
#include "Microsoft/Xna/Framework/Media/Song.hpp"

namespace HoneycombRush {

using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GameComponent;
using Microsoft::Xna::Framework::Audio::SoundEffect;
using Microsoft::Xna::Framework::Audio::SoundEffectInstance;
using Microsoft::Xna::Framework::Audio::SoundState;
using Microsoft::Xna::Framework::Media::MediaPlayer;
using Microsoft::Xna::Framework::Media::MediaState;
using Microsoft::Xna::Framework::Media::Song;

// Component that manages audio playback for all sounds. Port of
// Misc/AudioManager.cs.
class AudioManager : public GameComponent {
public:
    static AudioManager* Instance() { return instance_; }

    static void Initialize(Game& game) {
        instance_ = new AudioManager(game);
        game.getComponentsProperty().Add(instance_);
    }

    static void LoadSound(const std::string& contentName, const std::string& alias) {
        // SoundEffectInstance stores a raw `const SoundEffect*` back to the
        // SoundEffect it was created from (unlike XNA's GC-backed reference
        // types), so the SoundEffect must be cached here for as long as the
        // AudioManager itself lives -- a local temporary would dangle the
        // instant LoadSound() returns.
        auto [it, inserted] = instance_->soundEffects_.try_emplace(
            alias, instance_->getGameProperty().getContentProperty().Load<SoundEffect>(SoundAssetLocation + contentName));

        if (instance_->soundBank_.find(alias) == instance_->soundBank_.end()) {
            instance_->soundBank_.emplace(alias, it->second.CreateInstance());
        }
    }

    static void LoadSong(const std::string& contentName, const std::string& alias) {
        Song song = instance_->getGameProperty().getContentProperty().Load<Song>(SoundAssetLocation + contentName);

        if (instance_->musicBank_.find(alias) == instance_->musicBank_.end()) {
            instance_->musicBank_.emplace(alias, song);
        }
    }

    static void LoadSounds() {
        LoadSound("10SecondCountdown", "10SecondCountDown");
        LoadSound("30SecondWarning", "30SecondWarning");
        LoadSound("BeeBuzzing_Loop", "BeeBuzzing_Loop");
        LoadSound("Defeat", "Defeat");
        LoadSound("DepositingIntoVat_Loop", "DepositingIntoVat_Loop");
        LoadSound("FillingHoneyPot_Loop", "FillingHoneyPot_Loop");
        LoadSound("HighScore", "HighScore");
        LoadSound("HoneyPotBreak", "HoneyPotBreak");
        LoadSound("SmokeGun_Loop", "SmokeGun_Loop");
        LoadSound("Stung", "Stung");
        LoadSound("Stunned", "Stunned");
        LoadSound("Victory", "Victory");
    }

    static void LoadMusic() {
        LoadSong("InGameSong_Loop", "InGameSong_Loop");
        LoadSong("MenuMusic_Loop", "MenuMusic_Loop");
    }

    static void PlaySound(const std::string& soundName) {
        auto it = instance_->soundBank_.find(soundName);
        if (it != instance_->soundBank_.end()) {
            it->second.Play();
        }
    }

    static void PlaySound(const std::string& soundName, bool isLooped) {
        auto it = instance_->soundBank_.find(soundName);
        if (it != instance_->soundBank_.end()) {
            if (it->second.getIsLoopedProperty() != isLooped)
                it->second.setIsLoopedProperty(isLooped);
            it->second.Play();
        }
    }

    static void PlaySound(const std::string& soundName, bool isLooped, float volume) {
        auto it = instance_->soundBank_.find(soundName);
        if (it != instance_->soundBank_.end()) {
            if (it->second.getIsLoopedProperty() != isLooped)
                it->second.setIsLoopedProperty(isLooped);
            it->second.setVolumeProperty(volume);
            it->second.Play();
        }
    }

    static void StopSound(const std::string& soundName) {
        auto it = instance_->soundBank_.find(soundName);
        if (it != instance_->soundBank_.end()) {
            it->second.Stop();
        }
    }

    static void StopSounds() {
        for (auto& [name, sound] : instance_->soundBank_) {
            if (sound.getStateProperty() != SoundState::Stopped)
                sound.Stop();
        }
    }

    static void PauseResumeSounds(bool resumeSounds) {
        SoundState state = resumeSounds ? SoundState::Paused : SoundState::Playing;

        for (auto& [name, sound] : instance_->soundBank_) {
            if (sound.getStateProperty() == state) {
                if (resumeSounds)
                    sound.Resume();
                else
                    sound.Pause();
            }
        }
    }

    static void PlayMusic(const std::string& musicSoundName) {
        auto it = instance_->musicBank_.find(musicSoundName);
        if (it != instance_->musicBank_.end()) {
            if (MediaPlayer::getStateProperty() != MediaState::Stopped)
                MediaPlayer::Stop();

            MediaPlayer::setIsRepeatingProperty(true);
            MediaPlayer::Play(&it->second);
        }
    }

    static void StopMusic() {
        if (MediaPlayer::getStateProperty() != MediaState::Stopped)
            MediaPlayer::Stop();
    }

private:
    static constexpr const char* SoundAssetLocation = "Sounds/";

    explicit AudioManager(Game& game) : GameComponent(game) {}

    static inline AudioManager* instance_ = nullptr;

    std::unordered_map<std::string, SoundEffect> soundEffects_;
    std::unordered_map<std::string, SoundEffectInstance> soundBank_;
    std::unordered_map<std::string, Song> musicBank_;
};

} // namespace HoneycombRush
