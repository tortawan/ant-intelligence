#pragma once

#include <array>

namespace AIConfig {
    // Enumeration for object types present in the simulation
    enum class ObjectType {
        None = 0,
        Food = 1,
        Waste = 2,
        Egg = 3
    };

    // Enumeration for ant movement directions
    enum class Direction {
        North = 0,
        NorthEast,
        East,
        SouthEast,
        South,
        SouthWest,
        West,
        NorthWest
    };

    constexpr int NUM_DIRECTIONS = 8;

    // Default simulation parameters
    constexpr int DEFAULT_GROUND_WIDTH = 50;
    constexpr int DEFAULT_GROUND_LENGTH = 50;
    constexpr int DEFAULT_NUM_ANTS = 50;
    constexpr int DEFAULT_NUM_EXPERIMENTS = 1;
    constexpr int DEFAULT_ITERATIONS = 30001; // Reduced for quicker testing
    constexpr int DEFAULT_MEMORY_SIZE = 20;

    // Default sweep for similarity threshold
    constexpr int DEFAULT_THRESHOLD_START = 10;
    constexpr int DEFAULT_THRESHOLD_END = 20;
    constexpr int DEFAULT_THRESHOLD_INTERVAL = 15;

    // Default value for interaction cooldown (used if not sweeping)
    constexpr int DEFAULT_INTERACTION_COOLDOWN = 5;

    // --- NEW: Default sweep for interaction cooldown ---
    constexpr int DEFAULT_COOLDOWN_START = 5;
    constexpr int DEFAULT_COOLDOWN_END = 5;
    constexpr int DEFAULT_COOLDOWN_INTERVAL = 5;


    // Default probability range used for pick/drop logic
    constexpr std::array<double, 2> DEFAULT_PROB_RELU{ {0.3, 0.7} };
    // --- NEW: Default for video generation ---
    constexpr bool DEFAULT_VIDEO_ENABLED = true;
}
