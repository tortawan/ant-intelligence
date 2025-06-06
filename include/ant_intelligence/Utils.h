/**
 * @file Utils.h
 * @brief Contains general-purpose utility structures and functions for the simulation.
 */
#pragma once

#include <utility>    // For std::pair
#include <functional> // For std::hash

 /**
  * @struct pair_hash
  * @brief Custom hash function for std::pair<int, int>.
  * * This allows std::pair to be used as a key in unordered containers
  * like std::unordered_map and std::unordered_set.
  */
struct pair_hash {
    template <class T1, class T2>
    std::size_t operator() (const std::pair<T1, T2>& pair) const {
        // A simple and effective way to combine the hashes of the two pair elements.
        auto hash1 = std::hash<T1>{}(pair.first);
        auto hash2 = std::hash<T2>{}(pair.second);

        // A common way to combine hashes to reduce collisions.
        return hash1 ^ (hash2 << 1);
    }
};
