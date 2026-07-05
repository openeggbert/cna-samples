#pragma once

// StatisticsValue.hpp -- C++ port of RolePlayingGameData/Data/StatisticsValue.cs.

#include <algorithm>
#include <string>

namespace RolePlayingGameData {

// The set of relevant statistics for characters.
struct StatisticsValue {
    int HealthPoints = 0;
    int MagicPoints = 0;
    int PhysicalOffense = 0;
    int PhysicalDefense = 0;
    int MagicalOffense = 0;
    int MagicalDefense = 0;

    StatisticsValue() = default;
    StatisticsValue(int healthPoints, int magicPoints, int physicalOffense, int physicalDefense,
                     int magicalOffense, int magicalDefense)
        : HealthPoints(healthPoints), MagicPoints(magicPoints), PhysicalOffense(physicalOffense),
          PhysicalDefense(physicalDefense), MagicalOffense(magicalOffense), MagicalDefense(magicalDefense) {}

    bool IsZero() const {
        return HealthPoints == 0 && MagicPoints == 0 && PhysicalOffense == 0 && PhysicalDefense == 0 &&
               MagicalOffense == 0 && MagicalDefense == 0;
    }

    static StatisticsValue Add(const StatisticsValue& a, const StatisticsValue& b) {
        return StatisticsValue(a.HealthPoints + b.HealthPoints, a.MagicPoints + b.MagicPoints,
                                a.PhysicalOffense + b.PhysicalOffense, a.PhysicalDefense + b.PhysicalDefense,
                                a.MagicalOffense + b.MagicalOffense, a.MagicalDefense + b.MagicalDefense);
    }
    static StatisticsValue Subtract(const StatisticsValue& a, const StatisticsValue& b) {
        return StatisticsValue(a.HealthPoints - b.HealthPoints, a.MagicPoints - b.MagicPoints,
                                a.PhysicalOffense - b.PhysicalOffense, a.PhysicalDefense - b.PhysicalDefense,
                                a.MagicalOffense - b.MagicalOffense, a.MagicalDefense - b.MagicalDefense);
    }

    friend StatisticsValue operator+(const StatisticsValue& a, const StatisticsValue& b) { return Add(a, b); }
    friend StatisticsValue operator-(const StatisticsValue& a, const StatisticsValue& b) { return Subtract(a, b); }
    StatisticsValue& operator+=(const StatisticsValue& o) { *this = *this + o; return *this; }
    StatisticsValue& operator-=(const StatisticsValue& o) { *this = *this - o; return *this; }

    void ApplyMinimum(const StatisticsValue& min) {
        HealthPoints = std::max(HealthPoints, min.HealthPoints);
        MagicPoints = std::max(MagicPoints, min.MagicPoints);
        PhysicalOffense = std::max(PhysicalOffense, min.PhysicalOffense);
        PhysicalDefense = std::max(PhysicalDefense, min.PhysicalDefense);
        MagicalOffense = std::max(MagicalOffense, min.MagicalOffense);
        MagicalDefense = std::max(MagicalDefense, min.MagicalDefense);
    }
    void ApplyMaximum(const StatisticsValue& max) {
        HealthPoints = std::min(HealthPoints, max.HealthPoints);
        MagicPoints = std::min(MagicPoints, max.MagicPoints);
        PhysicalOffense = std::min(PhysicalOffense, max.PhysicalOffense);
        PhysicalDefense = std::min(PhysicalDefense, max.PhysicalDefense);
        MagicalOffense = std::min(MagicalOffense, max.MagicalOffense);
        MagicalDefense = std::min(MagicalDefense, max.MagicalDefense);
    }

    std::string ToString() const {
        return "HP:" + std::to_string(HealthPoints) + "; MP:" + std::to_string(MagicPoints) +
               "; PO:" + std::to_string(PhysicalOffense) + "; PD:" + std::to_string(PhysicalDefense) +
               "; MO:" + std::to_string(MagicalOffense) + "; MD:" + std::to_string(MagicalDefense);
    }

    std::string GetModifierString() const {
        std::string sb;
        bool first = true;
        auto append = [&](const char* label, int value) {
            if (value == 0) return;
            if (first) { first = false; } else { sb += "; "; }
            sb += label;
            sb += std::to_string(value);
        };
        append("HP:", HealthPoints);
        append("MP:", MagicPoints);
        append("PO:", PhysicalOffense);
        append("PD:", PhysicalDefense);
        append("MO:", MagicalOffense);
        append("MD:", MagicalDefense);
        return sb;
    }
};

} // namespace RolePlayingGameData
