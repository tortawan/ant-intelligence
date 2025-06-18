#include "ant_intelligence/Ground.h"
#include "ant_intelligence/Config.h"
#include "ant_intelligence/Objects.h"
#include <random>
#include <algorithm>
#include <numeric>
#include <queue>
#include <iostream>
#include <ctime>
#include <unordered_set>
#include <stdexcept>  // For std::invalid_argument
#include <cstdlib>

// Constructor: Build adjacency list of possible positions
Ground::Ground(int width,
    int length,
    const std::vector<double>& probabilities,
    const std::vector<double>& probRelu,
    int similarityThreshold,
    int interactionCooldown) // Added cooldown to constructor
    : width(width)
    , length(length)
    , probabilities(probabilities)
    , probRelu(probRelu)
    , similarityThreshold(similarityThreshold)
    , cooldown_duration(interactionCooldown) // Initialize new member
{
    if (width <= 0 || length <= 0) {
        throw std::invalid_argument("Invalid grid dimensions");
    }
    possiblePositions = getPossiblePositions();
}

void Ground::addAnt(int memorySize) {
    if (possiblePositions.empty()) {
        std::cerr << "Cannot add ant: No valid positions available." << std::endl;
        return;
    }

    auto position = getRandomPosition();
    Ant newAnt(position, width, length, true, memorySize);
    agents.push_back(newAnt);
}

void Ground::addObject(const std::unordered_map<std::shared_ptr<Object>, double>& objDict) {
    std::vector<std::shared_ptr<Object>> keys;
    std::vector<double> values;
    keys.reserve(objDict.size());
    values.reserve(objDict.size());

    for (auto& kv : objDict) {
        keys.push_back(kv.first);
        values.push_back(kv.second);
    }

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < length; ++y) {
            auto tempObject = getRandomObject(keys, values);
            if (tempObject) {
                obj[{x, y}] = tempObject;
            }
        }
    }
}

void Ground::moveAnts() {
    // OPTIMIZATION: Use a thread-local generator for thread-safe random numbers.
    thread_local static std::random_device rd;
    thread_local static std::mt19937 gen(rd());

    for (auto& ant : agents) {
        // Pass the generator to the move function.
        ant.move(possiblePositions, probabilities, gen);
    }
}

void Ground::assignWork() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    for (auto& ant : agents) {
        auto pos = ant.getPosition();
        auto it = obj.find(pos);
        auto groundObject = (it != obj.end()) ? it->second : nullptr;
        auto carried = ant.getLoad();

        // Update memory with the object the ant sees on the ground
        ant.updateMemory(groundObject);

        if (!carried) {
            if (groundObject) {
                int neighborCount = countNeighbors(pos, groundObject);
                double pickProb = reluRange(double(neighborCount) / possiblePositions[pos].size(),
                    probRelu[0], probRelu[1]);
                double randVal = dist(gen);
                if (randVal > pickProb) {
                    ant.setLoad(groundObject);
                    obj[pos] = nullptr;
                    // Update memory again when picking up an object
                    ant.updateMemory(groundObject);
                }
            }
        }
        else {
            // Update memory with the object the ant is carrying
            ant.updateMemory(carried);

            int neighborCount = countNeighbors(pos, carried);
            double dropProb = reluRange(double(neighborCount) / possiblePositions[pos].size(),
                probRelu[0], probRelu[1]);
            double randVal = dist(gen);
            if (randVal <= dropProb) {
                if (!groundObject) {
                    obj[pos] = carried;
                    ant.setLoad(nullptr);
                    // Update memory when dropping an object
                    ant.updateMemory(carried);
                }
                else {
                    obj[pos] = carried;
                    ant.setLoad(groundObject);
                    // Update memory when swapping objects
                    ant.updateMemory(carried);
                    ant.updateMemory(groundObject);
                }
            }
        }
    }
}


double Ground::averageClusterSize() {
    std::unordered_set<std::pair<int, int>, pair_hash> visitedLocations;
    visitedLocations.reserve(width * length);
    std::vector<double> clusterSizes;
    if (clusterSizes.capacity() < static_cast<size_t>((width * length) / 4)) {
        clusterSizes.reserve((width * length) / 4);
    }

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < length; ++y) {
            std::pair<int, int> pos = { x, y };

            if (visitedLocations.find(pos) != visitedLocations.end()) {
                continue;
            }

            auto it = obj.find(pos);
            if (it == obj.end() || !it->second) {
                visitedLocations.insert(pos);
                continue;
            }

            bool isValidType = std::dynamic_pointer_cast<Food>(it->second) ||
                std::dynamic_pointer_cast<Waste>(it->second) ||
                std::dynamic_pointer_cast<Egg>(it->second);
            if (!isValidType) {
                visitedLocations.insert(pos);
                continue;
            }

            double csize = bfsCluster(pos, it->second, visitedLocations);
            if (csize > 0) {
                clusterSizes.push_back(csize);
            }
        }
    }

    if (clusterSizes.empty()) {
        return 0.0;
    }

    double sum = std::accumulate(clusterSizes.begin(), clusterSizes.end(), 0.0);
    return sum / clusterSizes.size();
}

double Ground::bfsCluster(
    const std::pair<int, int>& startPos,
    std::shared_ptr<Object> targetObj,
    std::unordered_set<std::pair<int, int>, pair_hash>& visitedLocations
) {
    if (startPos.first < 0 || startPos.first >= width ||
        startPos.second < 0 || startPos.second >= length) {
        return 0.0;
    }

    double clusterSize = 0.0;
    std::vector<std::pair<int, int>> queue;
    queue.reserve(width * length);
    queue.push_back(startPos);
    size_t index = 0;

    while (index < queue.size()) {
        auto current = queue[index++];
        if (visitedLocations.find(current) != visitedLocations.end()) {
            continue;
        }
        visitedLocations.insert(current);

        auto it = obj.find(current);
        if (it != obj.end() && it->second == targetObj) {
            clusterSize += 1.0;
            for (auto& neighbor : possiblePositions[current]) {
                if (visitedLocations.find(neighbor) == visitedLocations.end()) {
                    queue.push_back(neighbor);
                }
            }
        }
    }
    return clusterSize;
}

#ifndef IS_TEST_BUILD
void Ground::showGround(const std::string& windowName, cv::VideoWriter& video) const {
    int scale = 6;
    cv::Mat image = cv::Mat::ones(width * scale, length * scale, CV_8UC3) * 255;

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < length; ++y) {
            auto it = obj.find({ x, y });
            if (it != obj.end() && it->second) {
                cv::Scalar color(128, 128, 128); // default gray
                if (std::dynamic_pointer_cast<Food>(it->second)) {
                    color = cv::Scalar(0, 255, 0);    // Green
                }
                else if (std::dynamic_pointer_cast<Egg>(it->second)) {
                    color = cv::Scalar(0, 255, 255);  // Yellow
                }
                else if (std::dynamic_pointer_cast<Waste>(it->second)) {
                    color = cv::Scalar(255, 0, 255);  // Magenta
                }
                cv::circle(
                    image,
                    cv::Point(y * scale + scale / 2, x * scale + scale / 2),
                    scale / 3,
                    color,
                    -1
                );
            }
        }
    }

    for (auto& ant : agents) {
        auto pos = ant.getPosition();
        cv::Scalar antColor(0, 0, 255); // Red
        cv::circle(
            image,
            cv::Point(pos.second * scale + scale / 2, pos.first * scale + scale / 2),
            scale / 3,
            antColor,
            -1
        );
    }

    cv::imshow(windowName, image);
    video.write(image);
    cv::waitKey(1);
}
#endif

void Ground::countObjects() const {
    int foodCount = 0;
    int eggCount = 0;
    int wasteCount = 0;

    for (auto& kv : obj) {
        if (kv.second) {
            if (std::dynamic_pointer_cast<Food>(kv.second)) {
                foodCount++;
            }
            else if (std::dynamic_pointer_cast<Egg>(kv.second)) {
                eggCount++;
            }
            else if (std::dynamic_pointer_cast<Waste>(kv.second)) {
                wasteCount++;
            }
        }
    }

    std::cout << "Number of Food objects: " << foodCount << std::endl;
    std::cout << "Number of Egg objects: " << eggCount << std::endl;
    std::cout << "Number of Waste objects: " << wasteCount << std::endl;
}

const std::vector<Ant>& Ground::getAgents() const {
    return agents;
}

std::unordered_map<std::pair<int, int>, std::vector<std::pair<int, int>>, pair_hash> Ground::getPossiblePositions() {
    // This defines the 8 directions relative to a central point.
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

std::pair<int, int> Ground::getRandomPosition() {
    thread_local static std::random_device rd;
    thread_local static std::mt19937 gen(rd());

    int size = static_cast<int>(possiblePositions.size());
    if (size == 0) {
        throw std::runtime_error("No possible positions available in Ground::getRandomPosition");
    }
    std::uniform_int_distribution<> distrib(0, size - 1);

    int index = distrib(gen);
    auto it = possiblePositions.begin();
    std::advance(it, index);
    return it->first;
}


std::shared_ptr<Object> Ground::getRandomObject(
    const std::vector<std::shared_ptr<Object>>& keys,
    const std::vector<double>& values
) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::discrete_distribution<> dist(values.begin(), values.end());

    int idx = dist(gen);
    if (!keys[idx]) {
        return nullptr;
    }
    return keys[idx];
}

double Ground::reluRange(double x, double a, double b) {
    if (x < a) {
        return 0.0;
    }
    else if (x > b) {
        return 1.0;
    }
    else {
        return (x - a) / (b - a);
    }
}

int Ground::countNeighbors(const std::pair<int, int>& pos, std::shared_ptr<Object> objType) {
    int count = 0;
    for (auto& neighbor : possiblePositions[pos]) {
        auto it = obj.find(neighbor);
        if (it != obj.end() && it->second == objType) {
            count++;
        }
    }
    return count;
}

static int getTypeFromLoad(const std::shared_ptr<Object>& load) {
    if (std::dynamic_pointer_cast<Food>(load)) {
        return static_cast<int>(AIConfig::ObjectType::Food);
    }
    else if (std::dynamic_pointer_cast<Waste>(load)) {
        return static_cast<int>(AIConfig::ObjectType::Waste);
    }
    else if (std::dynamic_pointer_cast<Egg>(load)) {
        return static_cast<int>(AIConfig::ObjectType::Egg);
    }
    return static_cast<int>(AIConfig::ObjectType::None);
}

void Ground::handleAntInteractions(int currentIteration) {
    std::unordered_map<std::pair<int, int>, std::vector<int>, pair_hash> positionsMap;
    for (size_t i = 0; i < agents.size(); ++i) {
        positionsMap[agents[i].getPosition()].push_back(static_cast<int>(i));
    }

    for (size_t i = 0; i < agents.size(); ++i) {
        Ant& antA = agents[i];
        if (antA.getInteractionCooldown() != 0 || !antA.getLoad())
            continue;

        auto posA = antA.getPosition();
        auto neighborCells = possiblePositions[posA];

        bool interactionOccurred = false;

        for (const auto& cell : neighborCells) {
            auto it = positionsMap.find(cell);
            if (it == positionsMap.end())
                continue;

            for (int j : it->second) {
                Ant& antB = agents[j];
                int loadType = getTypeFromLoad(antA.getLoad());

                int similarity = static_cast<int>(std::count(
                    antB.getMemory().begin(),
                    antB.getMemory().end(),
                    loadType
                ));

                if (similarity >= similarityThreshold) {
                    interactionCounter++;
                    antA.setPrevDirection((antB.getPrevDirection() + 4) % AIConfig::NUM_DIRECTIONS);
                    // Use the new configurable cooldown duration
                    antA.setInteractionCooldown(cooldown_duration);
                    interactionOccurred = true;
                    break;
                }
            }
            if (interactionOccurred) break;
        }
    }

    for (auto& ant : agents) {
        if (ant.getInteractionCooldown() > 0)
            ant.setInteractionCooldown(ant.getInteractionCooldown() - 1);
    }
}
