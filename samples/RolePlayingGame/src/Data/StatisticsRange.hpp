#pragma once

// StatisticsRange.hpp -- C++ port of RolePlayingGameData/Data/StatisticsRange.cs.

#include <string>

#include "Int32Range.hpp"
#include "StatisticsValue.hpp"
#include "System/Random.hpp"

namespace RolePlayingGameData {

// A range of character statistics values, typically used for constrained random modifiers.
struct StatisticsRange {
    Int32Range HealthPointsRange;
    Int32Range MagicPointsRange;
    Int32Range PhysicalOffenseRange;
    Int32Range PhysicalDefenseRange;
    Int32Range MagicalOffenseRange;
    Int32Range MagicalDefenseRange;

    StatisticsValue GenerateValue(System::Random& random) const {
        StatisticsValue v;
        v.HealthPoints = HealthPointsRange.GenerateValue(random);
        v.MagicPoints = MagicPointsRange.GenerateValue(random);
        v.PhysicalOffense = PhysicalOffenseRange.GenerateValue(random);
        v.PhysicalDefense = PhysicalDefenseRange.GenerateValue(random);
        v.MagicalOffense = MagicalOffenseRange.GenerateValue(random);
        v.MagicalDefense = MagicalDefenseRange.GenerateValue(random);
        return v;
    }

    static StatisticsRange Add(const StatisticsRange& a, const StatisticsValue& b) {
        StatisticsRange r;
        r.HealthPointsRange = a.HealthPointsRange + b.HealthPoints;
        r.MagicPointsRange = a.MagicPointsRange + b.MagicPoints;
        r.PhysicalOffenseRange = a.PhysicalOffenseRange + b.PhysicalOffense;
        r.PhysicalDefenseRange = a.PhysicalDefenseRange + b.PhysicalDefense;
        r.MagicalOffenseRange = a.MagicalOffenseRange + b.MagicalOffense;
        r.MagicalDefenseRange = a.MagicalDefenseRange + b.MagicalDefense;
        return r;
    }
    friend StatisticsRange operator+(const StatisticsRange& a, const StatisticsValue& b) { return Add(a, b); }
    StatisticsRange& operator+=(const StatisticsValue& b) { *this = *this + b; return *this; }

    std::string ToString() const {
        return "HP:" + HealthPointsRange.ToString() + "; MP:" + MagicPointsRange.ToString() +
               "; PO:" + PhysicalOffenseRange.ToString() + "; PD:" + PhysicalDefenseRange.ToString() +
               "; MO:" + MagicalOffenseRange.ToString() + "; MD:" + MagicalDefenseRange.ToString();
    }

    std::string GetModifierString() const {
        std::string sb;
        bool first = true;
        auto append = [&](const char* label, const Int32Range& r) {
            if (r.Minimum == 0 && r.Maximum == 0) return;
            if (first) { first = false; } else { sb += "; "; }
            sb += label;
            sb += r.ToString();
        };
        append("HP:", HealthPointsRange);
        append("MP:", MagicPointsRange);
        append("PO:", PhysicalOffenseRange);
        append("PD:", PhysicalDefenseRange);
        append("MO:", MagicalOffenseRange);
        append("MD:", MagicalDefenseRange);
        return sb;
    }
};

} // namespace RolePlayingGameData
