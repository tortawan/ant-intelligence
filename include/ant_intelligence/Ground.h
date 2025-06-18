#pragma once

/**
 * @file Ground.h
 * @brief Environment where ants and objects reside.
 */

#include "ant_intelligence/Ant.h"
#include "ant_intelligence/Objects.h"
#include "ant_intelligence/Config.h" // Added to get the default cooldown value
#include <vector>
#include <unordered_map>
#include <memory>
#include <unordered_set>

 // Conditionally include OpenCV headers only if this is NOT a test build.
#ifndef IS_TEST_BUILD
#include <opencv2/opencv.hpp>
#endif

/**
 * @class Ground
 * @brief Discrete grid that manages ants and objects.
 */
class Ground {
public:
    /**
     * @brief Construct a new Ground
     *
     * @param width               Grid width
     * @param length              Grid length
     * @param probabilities       Movement probabilities for ants
     * @param probRelu            Range used in interaction probability
     * @param similarityThreshold Number of matching memory items needed for interaction
     * @param interactionCooldown Number of steps an ant must wait after an interaction
     */
    Ground(int width,
        int length,
        const std::vector<double>& probabilities,
        const std::vector<double>& probRelu,
        int similarityThreshold,
        // FIX: Added a default value to the constructor for backward compatibility
        int interactionCooldown = AIConfig::DEFAULT_INTERACTION_COOLDOWN);

    /** @brief Create an ant and place it randomly on the ground */
    void addAnt(int memorySize = 20);
    /** @brief Fill the ground with objects according to the distribution */
    void addObject(const std::unordered_map<std::shared_ptr<Object>, double>& objDict);
    /** @brief Move all ants one step */
    void moveAnts();
    /** @brief Handle picking up and dropping objects */
    void assignWork();
    /** @brief Compute the average size of object clusters */
    double averageClusterSize();

    // Conditionally include visualization-related functions.
#ifndef IS_TEST_BUILD
    /** @brief Visualize the ground using OpenCV */
    void showGround(const std::string& windowName, cv::VideoWriter& video) const;
#endif

    /** @brief Print a count of all objects */
    void countObjects() const;
    /** @brief Access the underlying ant vector */
    const std::vector<Ant>& getAgents() const;
    /** @brief Check neighboring ants and update interaction counts */
    void handleAntInteractions(int currentIteration);


    /** @brief Number of interactions detected so far */
    int getInteractionCount() const { return interactionCounter; }

private:
    int width;
    int length;
    std::unordered_map<std::pair<int, int>, std::vector<std::pair<int, int>>, pair_hash> possiblePositions;
    std::vector<Ant> agents;
    std::unordered_map<std::pair<int, int>, std::shared_ptr<Object>, pair_hash> obj;
    std::vector<double> probabilities;
    std::vector<double> probRelu;
    int similarityThreshold;
    int cooldown_duration;
    int interactionCounter = 0;  // Counter for successful interactions

    /** @brief Pre-compute neighbour cells for each grid position */
    std::unordered_map<std::pair<int, int>, std::vector<std::pair<int, int>>, pair_hash> getPossiblePositions();
    /** @brief Pick a random valid grid cell */
    std::pair<int, int> getRandomPosition();
    /**
     * @brief Pick an object type according to the provided distribution.
     */
    std::shared_ptr<Object> getRandomObject(
        const std::vector<std::shared_ptr<Object>>& keys,
        const std::vector<double>& values);

    /** @brief Simple linear activation used for probabilities */
    double reluRange(double x, double a, double b);

    /**
     * @brief Breadth-first search to compute cluster sizes.
     */
    double bfsCluster(
        const std::pair<int, int>& startPos,
        std::shared_ptr<Object> targetObj,
        std::unordered_set<std::pair<int, int>, pair_hash>& visitedLocations);

    /** @brief Count neighbouring cells that contain the same object type */
    int countNeighbors(const std::pair<int, int>& pos, std::shared_ptr<Object> objType);
};
