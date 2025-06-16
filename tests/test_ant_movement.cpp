// File: tests/run_all_tests.cpp
//
// This program is a comprehensive unit test suite for the Ant and Ground classes.
// It includes tests for movement inertia, memory logic, interaction conditions,
// and boundary checking.
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
#include <random> // Added for std::mt19937
#include <deque>  // Added for std::deque

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
// OPTIMIZATION: Updated to test the std::deque-based memory
bool test_memory_fifo() {
    Ant testAnt({}, 0, 0, false, 3);
    auto food = std::make_shared<Food>();
    auto waste = std::make_shared<Waste>();
    auto egg = std::make_shared<Egg>();
    testAnt.updateMemory(food);
    testAnt.updateMemory(waste);
    testAnt.updateMemory(egg); // Memory is now full: {1, 2, 3}
    testAnt.updateMemory(food); // Oldest (1) is pushed out, new (1) is added. Memory: {2, 3, 1}

    // The expected state of the deque is {2, 3, 1}
    std::deque<int> expected_memory = { 2, 3, 1 };
    if (testAnt.getMemory() != expected_memory) {
        std::cout << "  [FAIL] FIFO logic failed. Expected {2, 3, 1}, Got: ";
        for (int i : testAnt.getMemory()) std::cout << i << " ";
        std::cout << std::endl;
        return false;
    }
    return true;
}

bool test_memory_ignores_nullptr() {
    Ant testAnt({}, 0, 0, false, 3);
    auto food = std::make_shared<Food>();
    testAnt.updateMemory(food);
    testAnt.updateMemory(nullptr); // Should have no effect

    std::deque<int> expected_memory = { 1 };
    if (testAnt.getMemory() != expected_memory) {
        std::cout << "  [FAIL] Nullptr test failed. Expected {1}, Got: ";
        for (int i : testAnt.getMemory()) std::cout << i << " ";
        std::cout << std::endl;
        return false;
    }
    return true;
}


// --- Test Case 3: Interaction Logic ---

// Helper function to create a specific scenario for interaction testing.
bool run_single_interaction_test(int threshold, int num_matching_memories) {
    Ground ground(10, 10, {}, {}, threshold);
    Ant antA({ 5, 5 }, 10, 10, false, 20);
    antA.setLoad(std::make_shared<Food>());
    Ant antB({ 5, 6 }, 10, 10, false, 20);
    for (int i = 0; i < num_matching_memories; ++i) {
        antB.updateMemory(std::make_shared<Food>());
    }
    int loadType = 1;
    auto& antB_memory = antB.getMemory(); // Get reference to the deque
    auto similarity = std::count(antB_memory.begin(), antB_memory.end(), loadType);
    return similarity >= threshold;
}


bool test_interaction_thresholds() {
    bool all_passed = true;
    for (int threshold = 0; threshold <= 20; ++threshold) {
        std::cout << "  Testing Threshold = " << threshold << std::endl;

        bool should_interact_result = run_single_interaction_test(threshold, threshold);
        if (!should_interact_result) {
            std::cout << "    [FAIL] Did not interact when memories == threshold." << std::endl;
            all_passed = false;
        }
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

// --- Test Case 4: Ant Movement at Boundaries ---

// Helper function to generate possible positions for a grid.
std::unordered_map<std::pair<int, int>, std::vector<std::pair<int, int>>, pair_hash>
get_test_possible_positions(int width, int length) {
    std::vector<std::pair<int, int>> neighborOffsets = {
        { 0, -1}, { 1, -1}, { 1,  0}, { 1,  1},
        { 0,  1}, {-1,  1}, {-1,  0}, {-1, -1}
    };
    std::unordered_map<std::pair<int, int>, std::vector<std::pair<int, int>>, pair_hash> result;

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < length; ++y) {
            std::vector<std::pair<int, int>> neighbors;
            for (auto& offset : neighborOffsets) {
                int nx = x + offset.first;
                int ny = y + offset.second;
                if (nx >= 0 && nx < width && ny >= 0 && ny < length) {
                    neighbors.push_back({ nx, ny });
                }
            }
            result[{x, y}] = neighbors;
        }
    }
    return result;
}

bool test_movement_at_boundaries() {
    const int width = 3;
    const int length = 3;
    const int numSamples = 100;
    bool all_passed = true;

    // Create a random number generator for this test.
    std::random_device rd;
    std::mt19937 gen(rd());

    auto possiblePositions = get_test_possible_positions(width, length);
    std::vector<double> uniform_prob(AIConfig::NUM_DIRECTIONS, 1.0 / AIConfig::NUM_DIRECTIONS);

    auto run_boundary_test = [&](const std::pair<int, int>& startPos, const std::string& posName) {
        const auto& legalNextPositions = possiblePositions.at(startPos);

        for (int i = 0; i < numSamples; ++i) {
            Ant sampleAnt(startPos, width, length, false, 5);
            // Pass the generator to the move function.
            sampleAnt.move(possiblePositions, uniform_prob, gen);
            auto newPos = sampleAnt.getPosition();

            bool found_in_legal_moves = false;
            for (const auto& legalPos : legalNextPositions) {
                if (newPos == legalPos) {
                    found_in_legal_moves = true;
                    break;
                }
            }

            if (!found_in_legal_moves) {
                std::cout << "  [FAIL] " << posName << ": Ant moved from (" << startPos.first << "," << startPos.second
                    << ") to an illegal position (" << newPos.first << "," << newPos.second << ")" << std::endl;
                all_passed = false;
                return;
            }
        }
        std::cout << "  [PASS] " << posName << ": Ant stayed in bounds for " << numSamples << " moves." << std::endl;
        };

    std::cout << "  Testing boundary conditions on a " << width << "x" << length << " grid." << std::endl;

    run_boundary_test({ 0, 0 }, "Corner (0,0)");
    run_boundary_test({ 1, 0 }, "Edge   (1,0)");
    run_boundary_test({ 1, 1 }, "Center (1,1)");
    run_boundary_test({ width - 1, length - 1 }, "Corner (2,2)");

    return all_passed;
}


int main() {
    TestSuite suite;

    suite.run("Movement Inertia", test_movement_inertia);
    suite.run("Memory FIFO Logic", test_memory_fifo);
    suite.run("Memory Ignores Null", test_memory_ignores_nullptr);
    suite.run("Interaction Logic by Threshold", test_interaction_thresholds);
    suite.run("Movement at Boundaries", test_movement_at_boundaries);

    suite.summary();

    return suite.failed > 0 ? 1 : 0;
}
