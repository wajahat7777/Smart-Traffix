#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <cmath>
#include <random>
#include <ctime>
#include <string>
#define TIMESTEP 0.01
#define SPEED 1/TIMESTEP
#define PI 3.14F
#ifndef VEHICLE_H
#define VEHICLE_H
// In vehicle.h#include <string>
class Vehicle {
protected:
    float x, y, dir;

public:
    // Constructor with parameters to initialize x, y, and dir
    Vehicle(float x, float y, float dir) : x(x), y(y), dir(dir) {}

    // Virtual move2() function to be overridden in derived classes
    virtual void move2() {
        // Base class implementation (if needed)
    }
    
    // Getter and setter functions for x, y, and dir
    float getX() const { return x; }
    float getY() const { return y; }
    float getDir() const { return dir; }

    void setX(float newX) { x = newX; }
    void setY(float newY) { y = newY; }
    void setDir(float newDir) { dir = newDir; }
    
};


#endif // VEHICLE_H

