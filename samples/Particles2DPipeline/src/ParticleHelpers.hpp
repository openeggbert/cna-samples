#pragma once

// ParticleHelpers.hpp — C++ port of ParticleHelpers.cs (XNA 4.0
// Particles2DPipeline sample).

#include "System/Random.hpp"

namespace Particles2DPipelineSample {

// Port of ParticleHelpers.cs.
namespace ParticleHelpers {

inline System::Random Random;

inline float RandomBetween(float min, float max) {
    return min + (float)Random.NextDouble() * (max - min);
}

} // namespace ParticleHelpers

} // namespace Particles2DPipelineSample
