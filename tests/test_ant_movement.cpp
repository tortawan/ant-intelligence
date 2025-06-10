// File: tests/run_all_tests.cpp
//
// This program is a comprehensive unit test suite for the Ant class.
// It includes tests for movement inertia and memory logic.
//
// How to compile (from the root project directory):
//   Using g++:
//   g++ -std=c++17 -Iinclude tests/run_all_tests.cpp src/Ant.cpp -o tests/ant_suite
//
//   Using MSVC (Visual Studio Command Prompt):
//   cl /EHsc /Iinclude tests/run_all_tests.cpp src/Ant.cpp /Fe:tests/ant_suite.exe
//
// How to run:
//   ./tests/ant_suite
//

#include "ant_intelligence/Ant.h"
#include "ant_intelligence/Config.h"
#include "ant_intelligence/Objects.h" // Needed for memory tests
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

    // Runs a test function and prints the result.
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

    // Prints the final summary of all tests.
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
    const int numSamplesPerDirection = 10000; // Reduced for speed, still statistically valid
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

bool test_memory_fifo() {
    // Create an ant with a small memory size of 3
    Ant testAnt({}, 0, 0, false, 3);

    auto food = std::make_shared<Food>();
    auto waste = std::make_shared<Waste>();
    auto egg = std::make_shared<Egg>();

    // 1. Add Food
    testAnt.updateMemory(food);
    if (testAnt.getMemory() != std::vector<int>{1}) {
        std::cout << "  Memory Test Fail: After adding Food." << std::endl;
        return false;
    }

    // 2. Add Waste
    testAnt.updateMemory(waste);
    if (testAnt.getMemory() != std::vector<int>{1, 2}) {
        std::cout << "  Memory Test Fail: After adding Waste." << std::endl;
        return false;
    }

    // 3. Fill memory with Egg
    testAnt.updateMemory(egg);
    if (testAnt.getMemory() != std::vector<int>{1, 2, 3}) {
        std::cout << "  Memory Test Fail: After adding Egg." << std::endl;
        return false;
    }

    // 4. Add another Food, pushing out the oldest item (the first Food)
    testAnt.updateMemory(food);
    // Expected memory: {Waste, Egg, Food} -> {2, 3, 1}
    if (testAnt.getMemory() != std::vector<int>{2, 3, 1}) {
        std::cout << "  Memory Test Fail: FIFO logic failed on overflow." << std::endl;
        return false;
    }

    std::cout << "  FIFO logic and object type mapping are correct." << std::endl;
    return true;
}

bool test_memory_ignores_nullptr() {
    Ant testAnt({}, 0, 0, false, 3);
    auto food = std::make_shared<Food>();

    testAnt.updateMemory(food);
    // Call updateMemory with a null object
    testAnt.updateMemory(nullptr);

    if (testAnt.getMemory() == std::vector<int>{1}) {
        std::cout << "  Correctly ignored nullptr." << std::endl;
        return true;
    }
    else {
        std::cout << "  Memory Test Fail: Memory was changed by a nullptr." << std::endl;
        return false;
    }
}


int main() {
    TestSuite suite;

    suite.run("Movement Inertia", test_movement_inertia);
    suite.run("Memory FIFO Logic", test_memory_fifo);
    suite.run("Memory Ignores Null", test_memory_ignores_nullptr);

    suite.summary();

    return suite.failed > 0 ? 1 : 0; // Return error code if any tests failed
}
