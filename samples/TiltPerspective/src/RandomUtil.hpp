#pragma once

// Port of RandomUtil.cs (XNA 4.0 TiltPerspective sample).
//
// The original defines `Random.NextFloat()`/`NextFloat(max)`/`NextFloat(min,max)`
// extension methods that avoid double-precision math via a raw bit trick
// (`(rng.Next() & 0x7fffffff) * (1/2147483648)`), plus a `[ThreadStatic]`
// SharedRandom accessor and a `NewRandom()` factory seeded from a GUID hash.
// None of that plumbing is needed for this single-threaded desktop port --
// per CLAUDE.md, `System::Random` is used directly (its own `NextDouble()`,
// not a hand-rolled bit trick) and a single local instance is constructed
// where the original would have reached for `RandomUtil.SharedRandom`
// (BallSimulation::AddBalls(), its only call site). Only the
// `NextFloat(min, max)` shape is kept, as a small free function.

#include "System/Random.hpp"

namespace TiltPerspectiveSample {

inline float NextFloat(System::Random& rng, float min, float max) {
    return static_cast<float>(rng.NextDouble()) * (max - min) + min;
}

} // namespace TiltPerspectiveSample
