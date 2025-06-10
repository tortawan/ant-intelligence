// File: tests/test_ant_movement.cpp
//
// This program is a comprehensive unit test to verify the "movement inertia"
// logic of the Ant class. It iterates through all 8 possible directions,
// running a statistical test on each to ensure the ant is most likely to
// continue moving in its previous direction.
//
// How to compile (from the root project directory):
//   Using g++:
//   g++ -std=c++17 -Iinclude tests/test_ant_movement.cpp src/Ant.cpp -o tests/ant_test
//
//   Using MSVC (Visual Studio Command Prompt):
//   cl /EHsc /Iinclude tests/test_ant_movement.cpp src/Ant.cpp /Fe:tests/ant_test.exe
//
// How to run:
//   ./tests/ant_test
//

#include "ant_intelligence/Ant.h"
#include "ant_intelligence/Config.h"
#include <iostream>
#include <vector>
#include <map>
#include <numeric>
#include <algorithm>

// Function to run a single test for a given previous direction
bool testDirection(int prevDirection, const std::vector<double>& probabilities, int numSamples) {
    std::cout << "\n--- Testing Direction: " << prevDirection << " ---" << std::endl;

    // Create an ant instance
    Ant testAnt;

    // Map to store counts of each new direction
    std::map<int, int> directionCounts;

    // Run the simulation loop
    for (int i = 0; i < numSamples; ++i) {
        int newDirection = testAnt.getRandomWeightedDirection(probabilities, prevDirection);
        directionCounts[newDirection]++;
    }

    // Find the most frequent direction
    auto mostFrequent = std::max_element(
        directionCounts.begin(),
        directionCounts.end(),
        [](const auto& a, const auto& b) {
            return a.second < b.second;
        }
    );

    int mostFrequentDirection = mostFrequent->first;
    int frequency = mostFrequent->second;

    std::cout << "Most frequent new direction: " << mostFrequentDirection
        << " (chosen " << frequency << " times out of " << numSamples << ")." << std::endl;

    return mostFrequentDirection == prevDirection;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Ant Movement Inertia Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    // --- Test Configuration ---
    const int numSamplesPerDirection = 100000;

    // Use the same probability distribution from the main simulation
    std::vector<double> prob = { 12, 5, 2, 1, 0.1, 1, 2, 5 };
    double prob_sum = std::accumulate(prob.begin(), prob.end(), 0.0);
    for (auto& p : prob) {
        p /= prob_sum;
    }

    int failures = 0;

    // --- Test Execution ---
    // Loop through all 8 directions and test each one.
    for (int dir = 0; dir < AIConfig::NUM_DIRECTIONS; ++dir) {
        if (testDirection(dir, prob, numSamplesPerDirection)) {
            std::cout << "[PASS] Test for direction " << dir << " succeeded." << std::endl;
        }
        else {
            std::cout << "[FAIL] Test for direction " << dir << " failed." << std::endl;
            failures++;
        }
    }

    // --- Final Summary ---
    std::cout << "\n--- Test Suite Summary ---" << std::endl;
    if (failures == 0) {
        std::cout << "[SUCCESS] All " << AIConfig::NUM_DIRECTIONS << " direction tests passed." << std::endl;
    }
    else {
        std::cout << "[FAILURE] " << failures << " out of " << AIConfig::NUM_DIRECTIONS << " tests failed." << std::endl;
    }

    return 0;
}
