#pragma once

// Direct port of RollingAverage.cs. To compensate for network latency, we need to know
// exactly how late each packet is. Trouble is, there is no guarantee that the clock will
// be set the same on every machine! The sender can include packet data indicating what
// time their clock showed when they sent the packet, but this is meaningless unless our
// local clock is in sync with theirs. To compensate for any clock skew, we maintain a
// rolling average of the send times from the last N incoming packets. If this average is,
// say, 50 milliseconds, but one specific packet arrives with a time difference of 70
// milliseconds, we can deduce this particular packet was delivered 20 milliseconds later
// than usual.

#include <vector>

namespace NetworkPrediction {

class RollingAverage {
public:
    explicit RollingAverage(int sampleCount) : sampleValues_(static_cast<std::size_t>(sampleCount), 0.0f) {}

    // Adds a new value to the rolling average, automatically replacing the oldest
    // existing entry.
    void AddValue(float newValue) {
        // To avoid having to recompute the sum from scratch every time we add a new
        // sample value, we just subtract out the value that we are replacing, then add
        // in the new value.
        valueSum_ -= sampleValues_[static_cast<std::size_t>(currentPosition_)];
        valueSum_ += newValue;

        // Store the new sample value.
        sampleValues_[static_cast<std::size_t>(currentPosition_)] = newValue;

        // Increment the write position.
        ++currentPosition_;

        // Track how many of the sampleValues elements are filled with valid data.
        if (currentPosition_ > sampleCount_)
            sampleCount_ = currentPosition_;

        // If we reached the end of the array, wrap back to the beginning.
        if (currentPosition_ >= static_cast<int>(sampleValues_.size())) {
            currentPosition_ = 0;

            // The trick we used at the top of this method to update the sum without
            // having to recompute it from scratch works pretty well to keep the average
            // efficient, but over time, floating point rounding errors could accumulate
            // enough to cause problems. To prevent that, we recalculate from scratch
            // each time the counter wraps.
            valueSum_ = 0.0f;

            for (float value : sampleValues_)
                valueSum_ += value;
        }
    }

    // Gets the current value of the rolling average.
    [[nodiscard]] float AverageValue() const {
        if (sampleCount_ == 0)
            return 0.0f;

        return valueSum_ / static_cast<float>(sampleCount_);
    }

private:
    // Array holding the N most recent sample values.
    std::vector<float> sampleValues_;

    // Counter indicating how many of the sampleValues have been filled up.
    int sampleCount_ = 0;

    // Cached sum of all the valid sampleValues.
    float valueSum_ = 0.0f;

    // Write position in the sampleValues array. When this reaches the end, it wraps
    // around, so we overwrite the oldest samples with newer data.
    int currentPosition_ = 0;
};

} // namespace NetworkPrediction
