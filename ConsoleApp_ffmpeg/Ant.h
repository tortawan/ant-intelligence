#pragma once

#include <utility>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <random>
#include <memory>
#include <sstream>  // For memory serialization
#include <string>   // For std::string

// Forward declarations
class Object;
class Food;
class Waste;
class Egg;


// Custom hash function for std::pair<int, int>
struct pair_hash {
    template <class T1, class T2>
    std::size_t operator() (const std::pair<T1, T2>& pair) const {
        // A simple combination of hashes for pair
        return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
    }
};

class Ant {
public:
    // Constructor
    // 'recordPath' enables storing visited positions
    Ant(std::pair<int, int> position = { -1, -1 },
        int width = 0,
        int length = 0,
        bool recordPath = false,
        int memorySize = 20);

    // Movement function: chooses a direction based on the provided probabilities
    void move(
        const std::unordered_map<std::pair<int, int>, std::vector<std::pair<int, int>>, pair_hash>& possiblePositions,
        const std::vector<double>& probabilities
    );

    // Getters
    std::pair<int, int> getPosition() const;
    std::shared_ptr<Object> getLoad() const; // The object the ant is carrying, if any
    const std::unordered_set<std::pair<int, int>, pair_hash>& getVisitedPositions() const;
    const std::vector<int>& getMemory() const;
    int getInteractionCooldown() const;
    int getPrevDirection() const;

    // Setters
    void setLoad(std::shared_ptr<Object> newLoad);
    void setRecordPath(bool record);
    void setInteractionCooldown(int cooldown);
    void setPrevDirection(int newDir);

    // New: Serialize memory to a comma-separated string
    std::string getMemoryString() const;

    // Updates memory based on encountered object
    void updateMemory(std::shared_ptr<Object> seenObject);

private:
    // Position in 2D grid
    std::pair<int, int> position;

    // Memory size
    int memorySize;

    // Boundaries
    int width;
    int length;

    // Movement dictionary and inverse
    std::unordered_map<int, std::pair<int, int>> movementDict;
    std::unordered_map<std::pair<int, int>, int, pair_hash> invMovementDict;

    // Tracks the previous direction (0..7)
    int prevDirection;

    // The object this ant is carrying
    std::shared_ptr<Object> load;

    // Memory: stores information about objects encountered
    // e.g. 1 for Food, 2 for Waste, 3 for Egg
    std::vector<int> memory;

    // Whether we record visited positions
    bool recordPath;

    // Set of visited positions (if recordPath == true)
    std::unordered_set<std::pair<int, int>, pair_hash> visitedPositions;

    // Interaction cooldown: steps remaining until ant can interact again
    int interactionCooldown;

    // Helpers
    void updateMovementDict();          // Initializes movementDict
    int getRandomDirection();           // Picks a random direction 0..7
    int getRandomWeightedDirection(
        const std::vector<double>& probabilities,
        int prevDirection
    ); // Weighted random direction

    
};
