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
    constexpr int DEFAULT_GROUND_WIDTH = 200;
    constexpr int DEFAULT_GROUND_LENGTH = 160;
    constexpr int DEFAULT_NUM_ANTS = 500;
    constexpr int DEFAULT_NUM_EXPERIMENTS = 4;
    constexpr int DEFAULT_ITERATIONS = 50001;
    constexpr int DEFAULT_MEMORY_SIZE = 20;
    constexpr int DEFAULT_THRESHOLD_START = 20;
    constexpr int DEFAULT_THRESHOLD_END = 20;
    constexpr int DEFAULT_THRESHOLD_INTERVAL = 5;

    // Default probability range used for pick/drop logic
    constexpr std::array<double, 2> DEFAULT_PROB_RELU{ {0.3, 0.7} };
}