#pragma once
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <cmath>
#include <random>
#include <ctime>
#include <pthread.h>
#include <queue>
#include <string>
#include <sstream>
#include <sys/wait.h>
#include <sys/time.h>
#include "i220776_D_roadtile.h"
#include "i220776_D_trafficlightgroup.h"

// Add breakdown probability constants
const float BREAKDOWN_BASE_PROBABILITY = 0.001f; // Base probability per update
const float MINIMUBREAKDOWNS = 1;                // Minimum number of breakdowns per simulation
// Conversion factor from km/h to pixels per move (assuming 1 pixel = 1 meter)
const float KMH_TO_PIXELS = 2.0;
// C
using namespace std;
using namespace sf;

// Lane configuration structure
struct LaneConfig
{
    float currentSpeed; // Current speed for all cars in this lane
    Clock speedClock;   // Timer for speed updates
    float spawnInterval;
    float car5Probability;
    float car5Interval;
};

// Car data structure
struct CarData
{
    string numberPlate;
    float speed; // in km/h
    bool challanStatus = false;
    int laneIndex; // Track which lane the car is in
    Clock speedUpdateClock;
    bool isBroken; // New field for breakdown status
    Vector2f breakdownPosition;
    bool hasSpawnedRescueVehicle = false; // New flag to track rescue vehicle spawn
    bool markedForDeletion;

    bool canMove(const TrafficLight *trafficLights)
    {

        return trafficLights[laneIndex].getState() == GREEN;
    }
};

// Remove duplicate enum and keep only one
enum tVehicleType
{
    CAR1,
    CAR2,
    CAR3,
    CAR4,
    CAR5,
    CAR6,
    CAR7
};

class Vehicle
{
protected:
    float x, y, dir;
    float speed;
    sf::Texture texture;
    sf::Sprite sprite;

public:
    Vehicle() : x(0), y(0), dir(0), speed(1.0f) {}
    Vehicle(float x, float y, float dir) : x(x), y(y), dir(dir), speed(1.0f) {}
    virtual void draw(sf::RenderWindow *window) = 0;
    virtual ~Vehicle() {}
};

class Car : public Vehicle
{
private:
    tVehicleType vehicleType;
    bool isBroken;
    sf::Color originalColor;
    bool challanStatus = false;

public:
    Car(tVehicleType type, float x, float y, float dir);
    void move2();
    void draw(sf::RenderWindow *window) override;
    void setSpeed(float newSpeed) { speed = newSpeed; }
    float getX() const { return x; }
    float getY() const { return y; }
    float getDir() const { return dir; }
    tVehicleType getType() const { return vehicleType; } // Inline definition
    float getSpeed() const { return speed; }
    void setBreakdownState(bool broken)
    {
        isBroken = broken;
        // Change car's appearance when broken
        if (broken)
        {
            sprite.setColor(sf::Color::Red); // Make the car red when broken
        }
        else
        {
            sprite.setColor(originalColor);
        }
    }

    bool isInBrokenState() const
    {
        return isBroken;
    }
    void resetSpeed()
    {
        speed = 0.0f; // Set speed to zero
    }
    void setChallanStatus(bool status)
    {
        challanStatus = status;
    }

    bool getChallanStatus() const
    {
        return challanStatus;
    }
    // Generate random number plate
    string generateNumberPlate()
    {
        static int plateNumber = 1000;
        stringstream ss;
        ss << "ABC-" << plateNumber++;
        return ss.str();
    }

    // Add this function to check if a lane is valid for CAR6
    bool isValidCar6Lane(int laneIndex)
    {
        // Only allow CAR6 in even-numbered lanes (lane2, lane4, lane6, lane8)
        return laneIndex % 2 == 1; // Index is 0-based, so odd index = even lane number
    }
};
