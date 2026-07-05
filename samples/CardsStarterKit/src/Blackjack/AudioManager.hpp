#pragma once

// AudioManager.hpp -- C++ port of Misc/AudioManager.cs (XNA 4.0
// CardsStarterKit sample). Static-singleton component managing sound/music
// playback. Follows the same pattern already established for NinjAcademy's
// AudioManager.hpp in this repo.
//
// LoadMusic()/PlayMusic() are ported faithfully but unreachable: the C#
// original's BlackjackGame.LoadContent() only ever calls LoadSounds(), never
// LoadMusic() -- and no music WAV/OGG assets ship in BlackjackHiDefContent
// either, so this is dead code in the original sample itself, not something
// introduced or fixed by this port. See missing.md.

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

#include "BlackjackCommon.hpp"

namespace Blackjack {

using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GameComponent;
using Microsoft::Xna::Framework::Audio::SoundEffect;
using Microsoft::Xna::Framework::Audio::SoundEffectInstance;
using Microsoft::Xna::Framework::Audio::SoundState;
using Microsoft::Xna::Framework::Media::MediaPlayer;
using Microsoft::Xna::Framework::Media::MediaState;
using Microsoft::Xna::Framework::Media::Song;

class AudioManager : public GameComponent {
public:
    static AudioManager* Instance() { return instance_; }

    static void Initialize(Game& game) {
        instance_ = new AudioManager(game);
        game.getComponentsProperty().Add(instance_);
    }

    static void LoadSound(const std::string& contentName, const std::string& alias) {
        auto [it, inserted] = instance_->soundEffects_.try_emplace(
            alias, instance_->getGameProperty().getContentProperty().Load<SoundEffect>(SoundAssetLocation + contentName));

        if (instance_->soundBank_.find(alias) == instance_->soundBank_.end())
            instance_->soundBank_.emplace(alias, it->second.CreateInstance());
    }

    static void LoadSong(const std::string& contentName, const std::string& alias) {
        Song song = instance_->getGameProperty().getContentProperty().Load<Song>(SoundAssetLocation + contentName);
        if (instance_->musicBank_.find(alias) == instance_->musicBank_.end())
            instance_->musicBank_.emplace(alias, song);
    }

    static void LoadSounds() {
        LoadSound("Bet", "Bet");
        LoadSound("CardFlip", "Flip");
        LoadSound("CardsShuffle", "Shuffle");
        LoadSound("Deal", "Deal");
    }

    // See file header: never actually called by BlackjackGame, kept for
    // parity with the original (which is equally unreachable there).
    static void LoadMusic() {
        LoadSong("InGameSong_Loop", "InGameSong_Loop");
        LoadSong("MenuMusic_Loop", "MenuMusic_Loop");
    }

    static void PlaySound(const std::string& soundName) {
        auto it = instance_->soundBank_.find(soundName);
        if (it != instance_->soundBank_.end())
            it->second.Play();
    }

    static void StopSound(const std::string& soundName) {
        auto it = instance_->soundBank_.find(soundName);
        if (it != instance_->soundBank_.end())
            it->second.Stop();
    }

    static void StopSounds() {
        for (auto& [name, sound] : instance_->soundBank_) {
            if (sound.getStateProperty() != SoundState::Stopped)
                sound.Stop();
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

} // namespace Blackjack
