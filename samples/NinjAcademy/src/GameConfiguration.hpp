#pragma once

// GameConfiguration.hpp — C++ port of NinjAcademyCommonTypes/GameConfiguration.cs
// (XNA 4.0 NinjAcademy sample), plus a hand-translated equivalent of
// Configuration/Configuration.xml (normally parsed by
// NinjAcademyPipeline/ConfigurationProcessor.cs at content-build time). CNA
// has no general XML content-pipeline deserializer, so -- matching this
// project's established precedent (e.g. DynamicMenu's MenuPage2.xml) -- the
// XML is hand-translated once into equivalent C++ construction code below;
// see missing.md.

#include <array>
#include <vector>

#include "System/TimeSpan.hpp"

namespace NinjAcademy {

// Describes the game's behavior during a specific stage. Port of
// NinjAcademyCommonTypes/GameConfiguration.cs's GamePhase.
struct GamePhase {
    // A negative duration indicates an infinite duration.
    System::TimeSpan Duration = System::TimeSpan::Zero;

    // Target appearance interval/probability per conveyor belt (upper, middle, lower).
    std::array<System::TimeSpan, 3> TargetAppearanceIntervals{};
    std::array<double, 3> TargetAppearanceProbabilities{};

    double GoldTargetProbablity = 0.0;

    System::TimeSpan BambooAppearanceInterval = System::TimeSpan::Zero;
    double BambooAppearanceProbablity = 0.0;

    System::TimeSpan DynamiteAppearanceInterval = System::TimeSpan::Zero;
    double DynamiteAppearanceProbablity = 0.0;

    // The i-th member is the probability of i+1 dynamite sticks being thrown.
    std::vector<double> DynamiteAmountProbabilities;
};

// Depicts the game's configuration. Port of
// NinjAcademyCommonTypes/GameConfiguration.cs's GameConfiguration.
struct GameConfiguration {
    int PlayerLives = 0;
    int PointsPerTarget = 0;
    int PointsPerGoldTarget = 0;
    int PointsPerBamboo = 0;
    std::vector<GamePhase> Phases;
};

// Hand-translated equivalent of Configuration/Configuration.xml (see file header).
inline GameConfiguration BuildConfiguration() {
    using System::TimeSpan;

    GameConfiguration config;
    config.PlayerLives = 5;
    config.PointsPerTarget = 1;
    config.PointsPerGoldTarget = 10;
    config.PointsPerBamboo = 3;

    {
        GamePhase phase;
        phase.Duration = TimeSpan::FromSeconds(45);
        phase.GoldTargetProbablity = 0.1;
        phase.TargetAppearanceIntervals = {TimeSpan::FromSeconds(5), TimeSpan::FromSeconds(5), TimeSpan::FromSeconds(5)};
        phase.TargetAppearanceProbabilities = {0.75, 0.75, 0.75};
        phase.BambooAppearanceInterval = TimeSpan::FromSeconds(2);
        phase.BambooAppearanceProbablity = 0.5;
        phase.DynamiteAppearanceInterval = TimeSpan::FromSeconds(10);
        phase.DynamiteAppearanceProbablity = 0.25;
        phase.DynamiteAmountProbabilities = {1.0};
        config.Phases.push_back(phase);
    }
    {
        GamePhase phase;
        phase.Duration = TimeSpan::FromSeconds(75);
        phase.GoldTargetProbablity = 0.1;
        phase.TargetAppearanceIntervals = {TimeSpan::FromSeconds(2), TimeSpan::FromSeconds(3), TimeSpan::FromSeconds(4)};
        phase.TargetAppearanceProbabilities = {0.35, 0.65, 0.35};
        phase.BambooAppearanceInterval = TimeSpan::FromSeconds(1);
        phase.BambooAppearanceProbablity = 0.65;
        phase.DynamiteAppearanceInterval = TimeSpan::FromSeconds(5);
        phase.DynamiteAppearanceProbablity = 0.5;
        phase.DynamiteAmountProbabilities = {0.2, 0.45, 0.35};
        config.Phases.push_back(phase);
    }
    {
        GamePhase phase;
        phase.Duration = TimeSpan::FromSeconds(-1);
        phase.GoldTargetProbablity = 0.1;
        phase.TargetAppearanceIntervals = {TimeSpan::FromSeconds(2), TimeSpan::FromSeconds(3), TimeSpan::FromSeconds(4)};
        phase.TargetAppearanceProbabilities = {0.5, 0.5, 0.5};
        phase.BambooAppearanceInterval = TimeSpan::FromSeconds(1);
        phase.BambooAppearanceProbablity = 0.75;
        phase.DynamiteAppearanceInterval = TimeSpan::FromSeconds(5);
        phase.DynamiteAppearanceProbablity = 0.5;
        phase.DynamiteAmountProbabilities = {0.25, 0.5, 0.25};
        config.Phases.push_back(phase);
    }

    return config;
}

} // namespace NinjAcademy
