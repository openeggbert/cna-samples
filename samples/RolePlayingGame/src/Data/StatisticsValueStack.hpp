#pragma once

// StatisticsValueStack.hpp -- C++ port of RolePlayingGameData/Data/StatisticsValueStack.cs.

#include <algorithm>
#include <vector>

#include "StatisticsValue.hpp"

namespace RolePlayingGameData {

// A collection of statistics that expire over time.
class StatisticsValueStack {
public:
    const StatisticsValue& TotalStatistics() const { return totalStatistics_; }

    // Add a new statistics, with a given duration, to the stack.
    // Entries with durations of 0 or less never expire.
    void AddStatistics(const StatisticsValue& statistics, int duration) {
        entries_.push_back({statistics, duration});
        CalculateTotalStatistics();
    }

    // Advance the stack and remove expired entries.
    void Advance() {
        entries_.erase(std::remove_if(entries_.begin(), entries_.end(),
                                       [](const Entry& e) { return e.remainingDuration == 1; }),
                        entries_.end());
        for (auto& e : entries_) e.remainingDuration--;
        CalculateTotalStatistics();
    }

private:
    struct Entry {
        StatisticsValue statistics;
        int remainingDuration;
    };

    void CalculateTotalStatistics() {
        totalStatistics_ = StatisticsValue();
        for (auto& e : entries_) totalStatistics_ += e.statistics;
    }

    std::vector<Entry> entries_;
    StatisticsValue totalStatistics_;
};

} // namespace RolePlayingGameData
