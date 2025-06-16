#pragma once

/**
 * @file Ant.h
 * @brief Definition of the Ant agent used in the simulation.
 */

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

/**
 * @class Ant
 * @brief Agent that moves around the grid and interacts with objects.
 */
class Ant {
public:
    /**
     * @brief Construct a new Ant
     *
     * @param position     Initial position on the grid
     * @param width        Grid width
     * @param length       Grid length
     * @param recordPath   Whether to record visited positions
     * @param memorySize   Capacity of the internal memory
     */
    Ant(std::pair<int, int> position = { -1, -1 },
        int width = 0,
        int length = 0,
        bool recordPath = false,
        int memorySize = 20);

    /**
     * @brief Move the ant according to the probability distribution.
     *
     * @param possiblePositions  Map of valid neighbor cells for every grid cell.
     * @param probabilities      Probability for choosing each of the 8 directions.
     * @param gen                The random number generator to use (for thread safety).
     */
    void move(
        const std::unordered_map<std::pair<int, int>, std::vector<std::pair<int, int>>, pair_hash>& possiblePositions,
        const std::vector<double>& probabilities,
        std::mt19937& gen);

    /** @name Getters */
    ///@{
    /** @brief Current position of the ant */
    std::pair<int, int> getPosition() const;
    /** @brief Object the ant is currently carrying */
    std::shared_ptr<Object> getLoad() const;
    /** @brief Collection of visited grid positions */
    const std::unordered_set<std::pair<int, int>, pair_hash>& getVisitedPositions() const;
    /** @brief Sequence of recently seen objects */
    const std::vector<int>& getMemory() const;
    /** @brief Steps remaining before the ant can interact again */
    int getInteractionCooldown() const;
    /** @brief Previous movement direction */
    int getPrevDirection() const;
    ///@}

    /** @name Setters */
    ///@{
    /** @brief Set the object carried by the ant */
    void setLoad(std::shared_ptr<Object> newLoad);
    /** @brief Enable or disable path recording */
    void setRecordPath(bool record);
    /** @brief Adjust the interaction cooldown */
    void setInteractionCooldown(int cooldown);
    /** @brief Explicitly set the previous movement direction */
    void setPrevDirection(int newDir);
    ///@}

    /**
     * @brief Serialize the internal memory to a comma separated string.
     */
    std::string getMemoryString() const;

    /**
     * @brief Update memory with information about a seen object.
     */
    void updateMemory(std::shared_ptr<Object> seenObject);

    /**
     * @brief Pick a direction using weighted probabilities.
     *
     * The distribution is rotated based on the previous direction to introduce
     * inertia in movement.
     */
    int getRandomWeightedDirection(
        const std::vector<double>& probabilities,
        int prevDirection);


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

    /** @name Helper functions */
    ///@{
    /** @brief Initialise the movement dictionary */
    void updateMovementDict();
    /** @brief Pick a random direction in range [0,7] */
    int getRandomDirection();
    ///@}


};
