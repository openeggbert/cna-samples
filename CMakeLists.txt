cmake_minimum_required(VERSION 3.20)

project(CNASamples LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# CNA dependency — consumed as a sibling add_subdirectory so that CNA's own
# CMake machinery (SDL vendoring, backend selection, etc.) is fully reused.
# ---------------------------------------------------------------------------

# Default to the Vulkan backend so CLion profiles and bare cmake invocations
# (which don't pass -DCNA_GRAPHICS_BACKEND) get Vulkan automatically.
# A preset or command-line -DCNA_GRAPHICS_BACKEND=<X> still wins because
# CACHE variables set earlier on the command line are not overwritten here.
if(NOT DEFINED CNA_GRAPHICS_BACKEND)
    set(CNA_GRAPHICS_BACKEND "EASYGL" CACHE STRING
        "Graphics backend to use (SDL_RENDERER, EASYGL, BGFX, or VULKAN)")
endif()

set(CNA_BUILD_TESTS         OFF CACHE BOOL "Build CNA tests"         FORCE)
set(CNA_BUILD_EXAMPLES      OFF CACHE BOOL "Build CNA examples"      FORCE)
set(SHARP_RUNTIME_BUILD_TESTS OFF CACHE BOOL "Build sharp-runtime tests" FORCE)

# CNA_Net/CNA_GamerServices (NetworkSession, GamerServicesComponent, Guide, ...) are
# separate CMake targets in cna, gated behind this option and OFF by default. Several
# samples (ClientServerSample, NetworkPrediction, PeerToPeer, and eventually NetRumble)
# need them.
set(CNA_ENABLE_NET          ON  CACHE BOOL "Enable CNA Net/GamerServices targets" FORCE)

add_subdirectory(../cna CNA_BUILD)

# Re-export the backend target name that CNA sets internally so that
# SampleHelpers.cmake can reference it when building linker groups.
# CNA sets BACKEND_TARGET in its own scope; we need it here.
# (If CNA doesn't export it, we fall back gracefully — no linker group.)
if(NOT DEFINED BACKEND_TARGET)
    # Attempt to derive it from the selected backend string.
    if(CNA_GRAPHICS_BACKEND STREQUAL "EASYGL")
        set(BACKEND_TARGET "easy-gl")
    elseif(CNA_GRAPHICS_BACKEND STREQUAL "VULKAN")
        set(BACKEND_TARGET "cna_backend_graphics_vulkan")
    endif()
endif()

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(SampleHelpers)

# ---------------------------------------------------------------------------
# Samples — add each subdirectory as it is implemented.
# Comment-out lines for samples that are not yet ported.
# ---------------------------------------------------------------------------

# Phase 1 — Foundation
add_subdirectory(samples/PrimitivesSample)
add_subdirectory(samples/Primitives3D)
add_subdirectory(samples/TexturesAndColors)
add_subdirectory(samples/ShapeRendering)
add_subdirectory(samples/GeneratedGeometry)
add_subdirectory(samples/SpriteEffects)
add_subdirectory(samples/SpriteSheet)
add_subdirectory(samples/SafeArea)
add_subdirectory(samples/InputSequence)
add_subdirectory(samples/InputReporter)

# Phase 2 — 2D Games  (uncomment as implemented)
add_subdirectory(samples/Platformer)
add_subdirectory(samples/CollisionSample)
add_subdirectory(samples/PerPixelCollision)
add_subdirectory(samples/TransformedCollision)
add_subdirectory(samples/RectangleCollision)
add_subdirectory(samples/Bounce)
add_subdirectory(samples/PathDrawing)
add_subdirectory(samples/Pathfinding)
add_subdirectory(samples/FlockingSample)
# add_subdirectory(samples/Spacewar)
add_subdirectory(samples/TicTacToe)
add_subdirectory(samples/ParticleSample)

# Phase 3 — 3D Graphics
add_subdirectory(samples/ChaseCamera)
add_subdirectory(samples/HeightmapCollision)
# add_subdirectory(samples/BloomSample)
# add_subdirectory(samples/ShadowMapping)
# add_subdirectory(samples/NormalMapping)
# add_subdirectory(samples/SkinningSample)
# add_subdirectory(samples/Particles3D)
add_subdirectory(samples/Particles2DPipeline)
# add_subdirectory(samples/BillboardSample)
# add_subdirectory(samples/InstancedModel)
add_subdirectory(samples/LensFlare)

# Phase 4 — Full Games
add_subdirectory(samples/MarbleMaze)
add_subdirectory(samples/RolePlayingGame)
# add_subdirectory(samples/NetRumble)
add_subdirectory(samples/HoneycombRush)
add_subdirectory(samples/NinjAcademy)
add_subdirectory(samples/CardsStarterKit)
# add_subdirectory(samples/ShipGame)

# Phase 5 — Audio, AI, Misc
add_subdirectory(samples/Audio3D)
add_subdirectory(samples/SoundAndMusic)
add_subdirectory(samples/ChaseAndEvade)
add_subdirectory(samples/AimingSample)
add_subdirectory(samples/FuzzyLogic)
add_subdirectory(samples/InverseKinematics)
add_subdirectory(samples/WaypointSample)
# add_subdirectory(samples/SplitScreen)
add_subdirectory(samples/CameraShake)

# Phase 6/7 — Game frameworks & UI
add_subdirectory(samples/GameStateManagement)
add_subdirectory(samples/CatapultWars)
add_subdirectory(samples/Yacht)
add_subdirectory(samples/GesturesSample)
add_subdirectory(samples/TouchThumbsticks)
add_subdirectory(samples/LocalizationSample)
add_subdirectory(samples/SnowShovel)
add_subdirectory(samples/DynamicMenu)
add_subdirectory(samples/UISample)
add_subdirectory(samples/PerformanceMeasuring)
add_subdirectory(samples/SoccerPitch)
add_subdirectory(samples/Orientation)

# ---------------------------------------------------------------------------
# Deferred / not-yet-ported samples — placeholder directories only
# (<Name>.htm + missing.md, no src/CMakeLists.txt yet). Uncomment and add a
# CMakeLists.txt once each sample's CNA blocker (see DEFERRED.md) is fixed and
# real source is added. See ignored.md for samples that will never appear here.
# ---------------------------------------------------------------------------

# Phase 1/2 stragglers missed by earlier add_subdirectory sweeps
# add_subdirectory(samples/ReachGraphicsDemo)
# add_subdirectory(samples/ColorReplacement)

# Phase 3 — 3D Graphics & Shaders (remaining)
# add_subdirectory(samples/DistortionSample)
# add_subdirectory(samples/NonPhotoRealistic)
# add_subdirectory(samples/PerPixelLighting)
# add_subdirectory(samples/VertexLighting)
# add_subdirectory(samples/RimLighting)
# add_subdirectory(samples/ShatterEffect)
# add_subdirectory(samples/XmlParticles)
add_subdirectory(samples/Graphics3D)
add_subdirectory(samples/PickingSample)
add_subdirectory(samples/TrianglePicking)

# Phase 4 — Models & Animation
# add_subdirectory(samples/SimpleAnimation)
# add_subdirectory(samples/CustomModelAnimation)
add_subdirectory(samples/CustomModelClass)
# add_subdirectory(samples/CustomModelEffect)
# add_subdirectory(samples/SkinnedModelExtensions)
# add_subdirectory(samples/CPUSkinning)

# Phase 6 — Full Games (remaining)
# add_subdirectory(samples/TankOnHeightmap)

# Deferred — Phone Hardware / Networking (PLAN.md appendix; the only 6 of ~27
# kept as future candidates — see ignored.md for the rest)
add_subdirectory(samples/AccelerometerSample)
add_subdirectory(samples/TiltPerspective)
add_subdirectory(samples/MicrophoneEcho)
add_subdirectory(samples/ClientServerSample)
add_subdirectory(samples/NetworkPrediction)
add_subdirectory(samples/PeerToPeer)
