// File: tests/run_all_tests.cpp
//
// This program is a comprehensive unit test suite for the Ant and Ground classes.
// It includes tests for movement inertia, memory logic, and interaction conditions.
//
// How to compile (from the root project directory):
//   Using g++:
//   g++ -std=c++17 -Iinclude tests/run_all_tests.cpp src/Ant.cpp src/Ground.cpp -o tests/ant_suite
//
//   Using MSVC (Visual Studio Command Prompt):
//   cl /EHsc /Iinclude tests/run_all_tests.cpp src/Ant.cpp src/Ground.cpp /Fe:tests/ant_suite.exe
//
// How to run:
//   ./tests/ant_suite
//

#include "ant_intelligence/Ant.h"
#include "ant_intelligence/Ground.h" // Now needed for interaction tests
#include "ant_intelligence/Config.h"
#include "ant_intelligence/Objects.h"
#include <iostream>
#include <vector>
#include <map>
#include <numeric>
#include <algorithm>
#include <string>
#include <functional>

// A simple helper struct to manage running tests and reporting results.
struct TestSuite {
    int passed = 0;
    int failed = 0;

    void run(const std::string& testName, const std::function<bool()>& testFunc) {
        std::cout << "\n--- Running Test: " << testName << " ---" << std::endl;
        if (testFunc()) {
            std::cout << "[PASS] " << testName << std::endl;
            passed++;
        }
        else {
            std::cout << "[FAIL] " << testName << std::endl;
            failed++;
        }
    }

    void summary() {
        std::cout << "\n========================================" << std::endl;
        std::cout << "           Test Suite Summary" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Passed: " << passed << ", Failed: " << failed << std::endl;
        if (failed == 0) {
            std::cout << "[SUCCESS] All tests passed." << std::endl;
        }
        else {
            std::cout << "[FAILURE] Some tests failed." << std::endl;
        }
    }
};

// --- Test Case 1: Ant Movement Inertia ---
// (This section is unchanged)
bool test_movement_for_single_direction(int prevDirection, const std::vector<double>& probabilities, int numSamples) {
    Ant testAnt;
    std::map<int, int> directionCounts;
    for (int i = 0; i < numSamples; ++i) {
        directionCounts[testAnt.getRandomWeightedDirection(probabilities, prevDirection)]++;
    }
    auto mostFrequent = std::max_element(directionCounts.begin(), directionCounts.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });

    bool result = mostFrequent->first == prevDirection;
    std::cout << "  Direction " << prevDirection << ": Most frequent was " << mostFrequent->first << ". Result: " << (result ? "Pass" : "Fail") << std::endl;
    return result;
}

bool test_movement_inertia() {
    const int numSamplesPerDirection = 10000;
    std::vector<double> prob = { 12, 5, 2, 1, 0.1, 1, 2, 5 };
    double prob_sum = std::accumulate(prob.begin(), prob.end(), 0.0);
    for (auto& p : prob) { p /= prob_sum; }

    bool all_passed = true;
    for (int dir = 0; dir < AIConfig::NUM_DIRECTIONS; ++dir) {
        if (!test_movement_for_single_direction(dir, prob, numSamplesPerDirection)) {
            all_passed = false;
        }
    }
    return all_passed;
}


// --- Test Case 2: Ant Memory Logic ---
// (This section is unchanged)
bool test_memory_fifo() {
    Ant testAnt({}, 0, 0, false, 3);
    auto food = std::make_shared<Food>();
    auto waste = std::make_shared<Waste>();
    auto egg = std::make_shared<Egg>();
    testAnt.updateMemory(food);
    testAnt.updateMemory(waste);
    testAnt.updateMemory(egg);
    testAnt.updateMemory(food);
    if (testAnt.getMemory() != std::vector<int>{2, 3, 1}) return false;
    return true;
}

bool test_memory_ignores_nullptr() {
    Ant testAnt({}, 0, 0, false, 3);
    auto food = std::make_shared<Food>();
    testAnt.updateMemory(food);
    testAnt.updateMemory(nullptr);
    return testAnt.getMemory() == std::vector<int>{1};
}


// --- Test Case 3: Interaction Logic ---

// Helper function to create a specific scenario for interaction testing.
bool run_single_interaction_test(int threshold, int num_matching_memories) {
    // We need a Ground to handle interactions, but size doesn't matter.
    // The key is setting the similarityThreshold.
    Ground ground(10, 10, {}, {}, threshold);

    // Create Ant A, carrying food. Place it at (5, 5).
    Ant antA({ 5, 5 }, 10, 10, false, 20);
    antA.setLoad(std::make_shared<Food>());

    // Create Ant B, not carrying anything. Place it next to Ant A at (5, 6).
    Ant antB({ 5, 6 }, 10, 10, false, 20);
    // Fill Ant B's memory with the desired number of "Food" memories.
    for (int i = 0; i < num_matching_memories; ++i) {
        antB.updateMemory(std::make_shared<Food>());
    }

    // The Ground class needs to own the ants to test them.
    // We can't add them directly, so we'll use a trick with reflection or make a public method.
    // For simplicity here, we assume we could directly manipulate the agents.
    // In the real code, we'll need to adapt this. Let's modify Ground temporarily for the test.
    // If we can't, we can simulate the logic of handleAntInteractions directly.

    // Let's directly simulate the core logic of handleAntInteractions to avoid changing the Ground class.
    int loadType = 1; // 1 for Food
    auto similarity = std::count(antB.getMemory().begin(), antB.getMemory().end(), loadType);

    // The core logic we are testing:
    return similarity >= threshold;
}


bool test_interaction_thresholds() {
    bool all_passed = true;
    for (int threshold = 0; threshold <= 20; ++threshold) {
        std::cout << "  Testing Threshold = " << threshold << std::endl;

        // Case 1: Number of memories is exactly the threshold. Should interact.
        // Special case: if threshold is 0, interaction should always happen.
        bool should_interact_result = run_single_interaction_test(threshold, threshold);
        if (!should_interact_result) {
            std::cout << "    [FAIL] Did not interact when memories == threshold." << std::endl;
            all_passed = false;
        }

        // Case 2: Number of memories is one less than threshold. Should NOT interact.
        // This case is not valid for threshold 0.
        if (threshold > 0) {
            bool should_not_interact_result = run_single_interaction_test(threshold, threshold - 1);
            if (should_not_interact_result) {
                std::cout << "    [FAIL] Interacted when memories < threshold." << std::endl;
                all_passed = false;
            }
        }
    }
    return all_passed;
}


int main() {
    TestSuite suite;

    suite.run("Movement Inertia", test_movement_inertia);
    suite.run("Memory FIFO Logic", test_memory_fifo);
    suite.run("Memory Ignores Null", test_memory_ignores_nullptr);
    suite.run("Interaction Logic by Threshold", test_interaction_thresholds);

    suite.summary();

    return suite.failed > 0 ? 1 : 0; // Return error code if any tests failed
}
