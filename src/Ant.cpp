#include "ant_intelligence/Ant.h"
#include "ant_intelligence/Objects.h"
#include "ant_intelligence/Config.h"
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
    const std::vector<double>& probabilities,
    std::mt19937& gen // Accept the generator by reference
) {
    if (possiblePositions.find(position) != possiblePositions.end()) {
        auto& nextStepsList = possiblePositions.at(position);
        if (nextStepsList.size() == AIConfig::NUM_DIRECTIONS) {
            int newDirection = getRandomWeightedDirection(probabilities, prevDirection);
            auto dx_dy = movementDict[newDirection];
            position.first += dx_dy.first;
            position.second += dx_dy.second;
            prevDirection = newDirection;
        }
        else {
            auto prevPosition = position;
            // OPTIMIZATION: Replaced non-thread-safe std::rand() with the modern generator.
            std::uniform_int_distribution<> distrib(0, nextStepsList.size() - 1);
            position = nextStepsList[distrib(gen)];

            auto dx = position.first - prevPosition.first;
            auto dy = position.second - prevPosition.second;

            // Handle potential issue where the new position is the same as the old one.
            // This can happen if an ant is completely trapped.
            if (invMovementDict.count({ dx, dy })) {
                prevDirection = invMovementDict.at(std::make_pair(dx, dy));
            }
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

// OPTIMIZATION: Changed return type to match deque
const std::deque<int>& Ant::getMemory() const {
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
        {static_cast<int>(AIConfig::Direction::North),      { 0, -1}},
        {static_cast<int>(AIConfig::Direction::NorthEast),  { 1, -1}},
        {static_cast<int>(AIConfig::Direction::East),       { 1,  0}},
        {static_cast<int>(AIConfig::Direction::SouthEast),  { 1,  1}},
        {static_cast<int>(AIConfig::Direction::South),      { 0,  1}},
        {static_cast<int>(AIConfig::Direction::SouthWest),  {-1,  1}},
        {static_cast<int>(AIConfig::Direction::West),       {-1,  0}},
        {static_cast<int>(AIConfig::Direction::NorthWest),  {-1, -1}}
    };
    invMovementDict.clear();
    for (const auto& kv : movementDict) {
        invMovementDict[kv.second] = kv.first;
    }
}

int Ant::getRandomDirection() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(
        0,
        static_cast<int>(AIConfig::Direction::NorthWest));
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
    AIConfig::ObjectType objectType = AIConfig::ObjectType::None;
    if (std::dynamic_pointer_cast<Food>(seenObject)) {
        objectType = AIConfig::ObjectType::Food;
    }
    else if (std::dynamic_pointer_cast<Waste>(seenObject)) {
        objectType = AIConfig::ObjectType::Waste;
    }
    else if (std::dynamic_pointer_cast<Egg>(seenObject)) {
        objectType = AIConfig::ObjectType::Egg;
    }
    if (objectType != AIConfig::ObjectType::None) {
        int objectTypeInt = static_cast<int>(objectType);

        // OPTIMIZATION: Use efficient deque operations instead of std::rotate.
        if (memory.size() >= memorySize) {
            memory.pop_front(); // Efficiently remove the oldest element.
        }
        memory.push_back(objectTypeInt); // Efficiently add the new element.
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
