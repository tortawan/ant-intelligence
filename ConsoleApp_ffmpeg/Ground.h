#pragma once

#include "Ant.h"
#include "Objects.h"
#include <vector>
#include <unordered_map>
#include <opencv2/opencv.hpp>
#include <memory>
#include <unordered_set>

class Ground {
public:
    Ground(int width,
        int length,
        const std::vector<double>& probabilities = std::vector<double>(8, 1.0 / 8),
        const std::vector<double>& probRelu = { 0.0, 0.5 },
        int similarityThreshold = 2);

    void addAnt(int memorySize = 20);
    void addObject(const std::unordered_map<std::shared_ptr<Object>, double>& objDict);
    void moveAnts();
    void assignWork();
    double averageClusterSize();
    void showGround(const std::string& windowName, cv::VideoWriter& video) const;
    void countObjects() const;
    const std::vector<Ant>& getAgents() const;
    void handleAntInteractions(int currentIteration);


    // Getter for interaction counter
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

    int interactionCounter = 0;  // Counter for successful interactions

    std::unordered_map<std::pair<int, int>, std::vector<std::pair<int, int>>, pair_hash> getPossiblePositions();
    std::pair<int, int> getRandomPosition();
    std::shared_ptr<Object> getRandomObject(
        const std::vector<std::shared_ptr<Object>>& keys,
        const std::vector<double>& values
    );

    double reluRange(double x, double a, double b);

    double bfsCluster(
        const std::pair<int, int>& startPos,
        std::shared_ptr<Object> targetObj,
        std::unordered_set<std::pair<int, int>, pair_hash>& visitedLocations
    );

    int countNeighbors(const std::pair<int, int>& pos, std::shared_ptr<Object> objType);
};
