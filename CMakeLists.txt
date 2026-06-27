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
    set(CNA_GRAPHICS_BACKEND "VULKAN" CACHE STRING
        "Graphics backend to use (SDL_RENDERER, EASYGL, BGFX, or VULKAN)")
endif()

set(CNA_BUILD_TESTS         OFF CACHE BOOL "Build CNA tests"         FORCE)
set(CNA_BUILD_EXAMPLES      OFF CACHE BOOL "Build CNA examples"      FORCE)
set(SHARP_RUNTIME_BUILD_TESTS OFF CACHE BOOL "Build sharp-runtime tests" FORCE)

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
# add_subdirectory(samples/PerPixelCollision)
add_subdirectory(samples/RectangleCollision)
add_subdirectory(samples/Bounce)
# add_subdirectory(samples/PathDrawing)
# add_subdirectory(samples/Pathfinding)
# add_subdirectory(samples/FlockingSample)
# add_subdirectory(samples/Spacewar)
# add_subdirectory(samples/TicTacToe)
# add_subdirectory(samples/ParticleSample)

# Phase 3 — 3D Graphics
# add_subdirectory(samples/ChaseCamera)
# add_subdirectory(samples/HeightmapCollision)
# add_subdirectory(samples/BloomSample)
# add_subdirectory(samples/ShadowMapping)
# add_subdirectory(samples/NormalMapping)
# add_subdirectory(samples/SkinningSample)
# add_subdirectory(samples/Particles3D)
# add_subdirectory(samples/BillboardSample)
# add_subdirectory(samples/InstancedModel)
# add_subdirectory(samples/LensFlare)

# Phase 4 — Full Games
# add_subdirectory(samples/MarbleMaze)
# add_subdirectory(samples/RolePlayingGame)
# add_subdirectory(samples/NetRumble)
# add_subdirectory(samples/HoneycombRush)
# add_subdirectory(samples/NinjAcademy)
# add_subdirectory(samples/ShipGame)

# Phase 5 — Audio, AI, Misc
# add_subdirectory(samples/Audio3D)
# add_subdirectory(samples/SoundAndMusic)
# add_subdirectory(samples/ChaseAndEvade)
# add_subdirectory(samples/FuzzyLogic)
# add_subdirectory(samples/InverseKinematics)
# add_subdirectory(samples/WaypointSample)
# add_subdirectory(samples/SplitScreen)
# add_subdirectory(samples/CameraShake)
