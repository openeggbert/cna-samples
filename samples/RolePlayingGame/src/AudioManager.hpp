#pragma once

// AudioManager.hpp -- adaptation of AudioManager.cs. The original wraps XACT
// (AudioEngine/SoundBank/WaveBank/Cue) and loads a compiled .xgs/.xwb/.xsb
// project -- CNA actually implements that XACT API (see cna's
// Microsoft::Xna::Framework::Audio::AudioEngine), but this sample's source
// tree only ships the *uncompiled* .xap XACT project plus raw .wav files, and
// there is no XACT authoring tool available to compile real .xgs/.xwb/.xsb
// binaries from it. So this port keeps the original's cue-name-based public
// API (PlayCue/PushMusic/PopMusic/StopMusic) but backs it with plain
// SoundEffect/SoundEffectInstance, lazily loading each cue's .wav by name the
// first time it's requested (matching every cue name referenced by
// Gear/Spell/Screen code) rather than a fixed pre-registered list. See
// missing.md.

#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "Microsoft/Xna/Framework/Audio/SoundEffect.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundEffectInstance.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundState.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"

namespace RolePlaying {

using Microsoft::Xna::Framework::Content::ContentManager;
using Microsoft::Xna::Framework::Audio::SoundEffect;
using Microsoft::Xna::Framework::Audio::SoundEffectInstance;
using Microsoft::Xna::Framework::Audio::SoundState;

class AudioManager {
public:
    static void Initialize(ContentManager& content) { content_ = &content; }

    static void PlayCue(const std::string& cueName) {
        if (cueName.empty()) return;
        if (auto* instance = GetInstance(cueName)) instance->Play();
    }

    static void PlayMusic(const std::string& cueName) {
        musicStack_ = {};
        PushMusic(cueName);
    }

    static void PushMusic(const std::string& cueName) {
        if (cueName.empty()) return;
        musicStack_.push(cueName);
        if (currentMusicName_ != cueName) {
            StopCurrentMusic();
            currentMusicName_ = cueName;
            if (auto* instance = GetInstance(cueName)) {
                instance->setIsLoopedProperty(true);
                instance->Play();
            }
        }
    }

    static void PopMusic() {
        std::string nextName;
        if (!musicStack_.empty()) {
            musicStack_.pop();
            if (!musicStack_.empty()) nextName = musicStack_.top();
        }
        if (currentMusicName_ != nextName) {
            StopCurrentMusic();
            currentMusicName_ = nextName;
            if (!nextName.empty()) {
                if (auto* instance = GetInstance(nextName)) {
                    instance->setIsLoopedProperty(true);
                    instance->Play();
                }
            }
        }
    }

    static void StopMusic() {
        musicStack_ = {};
        StopCurrentMusic();
    }

private:
    static void StopCurrentMusic() {
        if (!currentMusicName_.empty()) {
            auto it = instances_.find(currentMusicName_);
            if (it != instances_.end() && it->second.getStateProperty() != SoundState::Stopped) it->second.Stop();
        }
        currentMusicName_.clear();
    }

    // Returns nullptr if the cue's .wav is missing or fails to load -- this
    // sample's shipped asset tree is missing a handful of cue names
    // referenced by data (e.g. "BeachTheme"); the original silently no-ops
    // in the equivalent case (NoAudioHardwareException), so this port does
    // the same rather than crash. See missing.md.
    static SoundEffectInstance* GetInstance(const std::string& cueName) {
        auto it = instances_.find(cueName);
        if (it != instances_.end()) return &it->second;
        if (missingCues_.count(cueName)) return nullptr;
        try {
            auto sound = content_->Load<SoundEffect>("Audio/" + cueName);
            auto [inserted, ok] = instances_.emplace(cueName, sound.CreateInstance());
            (void)ok;
            return &inserted->second;
        } catch (const std::exception&) {
            missingCues_.insert(cueName);
            return nullptr;
        }
    }

    static inline ContentManager* content_ = nullptr;
    static inline std::unordered_map<std::string, SoundEffectInstance> instances_;
    static inline std::unordered_set<std::string> missingCues_;
    static inline std::stack<std::string> musicStack_;
    static inline std::string currentMusicName_;
};

} // namespace RolePlaying
