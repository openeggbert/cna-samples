#pragma once

// Int32Range.hpp -- C++ port of RolePlayingGameData/Data/Int32Range.cs.

#include <string>

#include "System/Random.hpp"

namespace RolePlayingGameData {

// Defines a range of values, useful for generating values in that range.
struct Int32Range {
    int Minimum = 0;
    int Maximum = 0;

    Int32Range() = default;
    Int32Range(int minimum, int maximum) : Minimum(minimum), Maximum(maximum) {}

    int Average() const { return Minimum + Range() / 2; }
    int Range() const { return Maximum - Minimum; }

    // Generate a random value between the minimum and maximum, inclusively.
    int GenerateValue(System::Random& random) const { return random.Next(Minimum, Maximum); }

    static Int32Range Add(const Int32Range& a, const Int32Range& b) {
        return Int32Range(a.Minimum + b.Minimum, a.Maximum + b.Maximum);
    }
    static Int32Range Subtract(const Int32Range& a, const Int32Range& b) {
        return Int32Range(a.Minimum - b.Minimum, a.Maximum - b.Maximum);
    }
    static Int32Range Add(const Int32Range& r, int amount) {
        return Int32Range(r.Minimum + amount, r.Maximum + amount);
    }
    static Int32Range Subtract(const Int32Range& r, int amount) {
        return Int32Range(r.Minimum - amount, r.Maximum - amount);
    }

    friend Int32Range operator+(const Int32Range& a, const Int32Range& b) { return Add(a, b); }
    friend Int32Range operator-(const Int32Range& a, const Int32Range& b) { return Subtract(a, b); }
    friend Int32Range operator+(const Int32Range& r, int amount) { return Add(r, amount); }
    friend Int32Range operator-(const Int32Range& r, int amount) { return Subtract(r, amount); }
    Int32Range& operator+=(const Int32Range& other) { *this = *this + other; return *this; }

    std::string ToString() const {
        return "(" + std::to_string(Minimum) + "," + std::to_string(Maximum) + ")";
    }
};

} // namespace RolePlayingGameData
