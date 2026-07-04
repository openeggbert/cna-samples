#pragma once

// ConfigurationManager.hpp — C++ port of Misc/ConfigurationManager.cs (XNA 4.0
// HoneycombRush sample).
//
// Adaptation note (see missing.md): the original loads Configuration.xml at
// runtime via System.Xml.Linq.XDocument. Per this project's established
// "no general XML deserializer, hand-translate once" pattern, the three
// difficulty configurations are hand-translated into this static table
// instead — the values are copied verbatim from Configuration.xml.

#include <optional>
#include <unordered_map>

#include "System/TimeSpan.hpp"

namespace HoneycombRush {

enum class DifficultyMode {
    Easy = 1,
    Medium = 2,
    Hard = 3
};

inline DifficultyMode& operator++(DifficultyMode& mode) {
    mode = static_cast<DifficultyMode>(static_cast<int>(mode) + 1);
    return mode;
}

struct Configuration {
    System::TimeSpan GameElapsed;

    float MinWorkerBeeVelocity = 0.0f;
    float MaxWorkerBeeVelocity = 0.0f;
    float MinSoldierBeeVelocity = 0.0f;
    float MaxSoldierBeeVelocity = 0.0f;

    int TotalSmokeAmount = 0;
    int DecreaseAmountSpeed = 0;
    int IncreaseAmountSpeed = 0;
    int HighScoreFactor = 0;
};

// Supplies access to configuration data. Port of Misc/ConfigurationManager.cs.
class ConfigurationManager {
public:
    static const std::unordered_map<DifficultyMode, Configuration>& ModesConfiguration() {
        return GetModesConfiguration();
    }

    static bool IsLoaded() { return true; }

    static std::optional<DifficultyMode>& CurrentDifficultyMode() {
        static std::optional<DifficultyMode> mode;
        return mode;
    }

private:
    static const std::unordered_map<DifficultyMode, Configuration>& GetModesConfiguration() {
        static const std::unordered_map<DifficultyMode, Configuration> modes = [] {
            std::unordered_map<DifficultyMode, Configuration> m;

            Configuration easy;
            easy.GameElapsed = System::TimeSpan(0, 2, 0);
            easy.MinWorkerBeeVelocity = 1;
            easy.MaxWorkerBeeVelocity = 3;
            easy.MinSoldierBeeVelocity = 2;
            easy.MaxSoldierBeeVelocity = 4;
            easy.TotalSmokeAmount = 50;
            easy.DecreaseAmountSpeed = 2;
            easy.IncreaseAmountSpeed = 1;
            easy.HighScoreFactor = 1;
            m[DifficultyMode::Easy] = easy;

            Configuration medium;
            medium.GameElapsed = System::TimeSpan(0, 1, 30);
            medium.MinWorkerBeeVelocity = 2;
            medium.MaxWorkerBeeVelocity = 4;
            medium.MinSoldierBeeVelocity = 3;
            medium.MaxSoldierBeeVelocity = 5;
            medium.TotalSmokeAmount = 40;
            medium.DecreaseAmountSpeed = 2;
            medium.IncreaseAmountSpeed = 1;
            medium.HighScoreFactor = 2;
            m[DifficultyMode::Medium] = medium;

            Configuration hard;
            hard.GameElapsed = System::TimeSpan(0, 1, 0);
            hard.MinWorkerBeeVelocity = 2;
            hard.MaxWorkerBeeVelocity = 6;
            hard.MinSoldierBeeVelocity = 3;
            hard.MaxSoldierBeeVelocity = 6;
            hard.TotalSmokeAmount = 30;
            hard.DecreaseAmountSpeed = 2;
            hard.IncreaseAmountSpeed = 1;
            hard.HighScoreFactor = 3;
            m[DifficultyMode::Hard] = hard;

            return m;
        }();
        return modes;
    }
};

} // namespace HoneycombRush
