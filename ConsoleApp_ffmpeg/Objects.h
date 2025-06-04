#pragma once

#include <memory>

// Base Object class
class Object {
public:
    virtual ~Object() = default;
};

// Derived classes
class Food : public Object {
public:
    ~Food() override = default;
};

class Waste : public Object {
public:
    ~Waste() override = default;
};

class Egg : public Object {
public:
    ~Egg() override = default;
};
