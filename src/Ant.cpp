#include "ant_intelligence/Ant.h"
#include "ant_intelligence/Objects.h"
#include <algorithm>
#include <random>
#include <chrono>
#include <cstdlib>

Ant::Ant(std::pair<int, int> position, int width, int length, bool recordPath, int memorySize)
    : position(position)
    , width(width)
    , length(length)
    , prevDirection(0)
    , load(nullptr)
    , recordPath(recordPath)
    , interactionCooldown(0)
    , memorySize(memorySize) // Initialize the memory size
{
    // Initialize the movement dictionary
    updateMovementDict();
    // Get a random initial direction
    prevDirection = getRandomDirection();
}

void Ant::move(
    const std::unordered_map<std::pair<int, int>, std::vector<std::pair<int, int>>, pair_hash>& possiblePositions,
    const std::vector<double>& probabilities
) {
    if (possiblePositions.find(position) != possiblePositions.end()) {
        auto nextStepsList = possiblePositions.at(position);
        if (nextStepsList.size() == 8) {
            int newDirection = getRandomWeightedDirection(probabilities, prevDirection);
            auto dx_dy = movementDict[newDirection];
            position.first += dx_dy.first;
            position.second += dx_dy.second;
            prevDirection = newDirection;
        }
        else {
            auto prevPosition = position;
            position = nextStepsList[std::rand() % nextStepsList.size()];
            auto dx = position.first - prevPosition.first;
            auto dy = position.second - prevPosition.second;
            prevDirection = invMovementDict.at(std::make_pair(dx, dy));
        }
        if (recordPath) {
            visitedPositions.insert(position);
        }
    }
}

std::pair<int, int> Ant::getPosition() const {
    return position;
}

std::shared_ptr<Object> Ant::getLoad() const {
    return load;
}

void Ant::setLoad(std::shared_ptr<Object> newLoad) {
    load = newLoad;
}

const std::unordered_set<std::pair<int, int>, pair_hash>& Ant::getVisitedPositions() const {
    return visitedPositions;
}

void Ant::setRecordPath(bool record) {
    recordPath = record;
}

const std::vector<int>& Ant::getMemory() const {
    return memory;
}

int Ant::getInteractionCooldown() const {
    return interactionCooldown;
}

void Ant::setInteractionCooldown(int cooldown) {
    interactionCooldown = cooldown;
}

int Ant::getPrevDirection() const {
    return prevDirection;
}

void Ant::setPrevDirection(int newDir) {
    prevDirection = newDir;
}

void Ant::updateMovementDict() {
    movementDict = {
        {0, { 0, -1}}, // North
        {1, { 1, -1}}, // Northeast
        {2, { 1,  0}}, // East
        {3, { 1,  1}}, // Southeast
        {4, { 0,  1}}, // South
        {5, {-1,  1}}, // Southwest
        {6, {-1,  0}}, // West
        {7, {-1, -1}}  // Northwest
    };
    invMovementDict.clear();
    for (const auto& kv : movementDict) {
        invMovementDict[kv.second] = kv.first;
    }
}

int Ant::getRandomDirection() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 7);
    return dis(gen);
}

int Ant::getRandomWeightedDirection(const std::vector<double>& probabilities, int prevDirection) {
    std::vector<double> shifted(probabilities);
    int size = static_cast<int>(probabilities.size());
    int n = prevDirection % size;
    std::rotate(shifted.rbegin(), shifted.rbegin() + n, shifted.rend());
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::discrete_distribution<> distribution(shifted.begin(), shifted.end());
    return distribution(gen);
}

void Ant::updateMemory(std::shared_ptr<Object> seenObject) {
    if (!seenObject) {
        return;
    }
    int objectType = 0;
    if (std::dynamic_pointer_cast<Food>(seenObject)) {
        objectType = 1;
    }
    else if (std::dynamic_pointer_cast<Waste>(seenObject)) {
        objectType = 2;
    }
    else if (std::dynamic_pointer_cast<Egg>(seenObject)) {
        objectType = 3;
    }
    if (objectType != 0) {
        if (memory.size() >= memorySize) {
            std::rotate(memory.begin(), memory.begin() + 1, memory.end());
            memory.back() = objectType;
        }
        else {
            memory.push_back(objectType);
        }
    }
}

// New: Serialize memory to a comma-separated string.
std::string Ant::getMemoryString() const {
    std::stringstream ss;
    for (int mem : memory) {
        ss << mem << ",";
    }
    return ss.str();
}
