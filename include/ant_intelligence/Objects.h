/**
 * @file Objects.h
 * @brief Defines the object types that can exist on the ground.
 */
#pragma once

#include <memory>

 /**
  * @class Object
  * @brief Base class for all items that can be picked up by ants.
  */
class Object {
public:
    virtual ~Object() = default;
};

/**
 * @class Food
 * @brief Represents a food item.
 */
class Food : public Object {};

/**
 * @class Waste
 * @brief Represents a waste item.
 */
class Waste : public Object {};

/**
 * @class Egg
 * @brief Represents an egg item.
 */
class Egg : public Object {};