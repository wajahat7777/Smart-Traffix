#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <cmath>
#include <random>
#include <ctime>
#include <pthread.h>
#include <queue>
#include "i220776_D_roadtile.h"
#include "i220776_D_trafficlightgroup.h"
#include "i220776_D_car.h"
#include "i220776_D_SmartTraffix.h"
#include <sstream>
#include <sys/wait.h>
#include <sys/time.h>

// Random number generator setup
random_device rd;
mt19937 gen(rd());
uniform_real_distribution<> dis(0, 1);

// Mutex for thread-safe result sharing
pthread_mutex_t spawnCheckMutex = PTHREAD_MUTEX_INITIALIZER;

// Thread argument structure for spawn checking
struct SpawnCheckArgs
{
    int carCount;
    Car **cars;
    const float (*spawnPositions)[3];
    int spawnIndex;
    float minDistance;
    bool *canSpawn;
    int startIndex;
    int endIndex;
};

// Thread function for checking car spawn conditions
void *checkSpawnConditionsThread(void *args)
{
    SpawnCheckArgs *threadArgs = static_cast<SpawnCheckArgs *>(args);
    bool localCanSpawn = true;

    for (int j = threadArgs->startIndex; j < threadArgs->endIndex; j++)
    {
        // Skip if cars array element is null
        if (threadArgs->cars[j] == nullptr)
            continue;

        // Check if cars are in the same direction
        if (threadArgs->spawnPositions[threadArgs->spawnIndex][2] == threadArgs->cars[j]->getDir())
        {
            float dx = threadArgs->spawnPositions[threadArgs->spawnIndex][0] - threadArgs->cars[j]->getX();
            float dy = threadArgs->spawnPositions[threadArgs->spawnIndex][1] - threadArgs->cars[j]->getY();

            float distance = sqrt(dx * dx + dy * dy);

            // If distance is less than minimum, mark as cannot spawn
            if (distance < threadArgs->minDistance)
            {
                pthread_mutex_lock(&spawnCheckMutex);
                *threadArgs->canSpawn = false;
                pthread_mutex_unlock(&spawnCheckMutex);
                break;
            }
        }
    }

    return nullptr;
}

// Threaded version of canSpawnCar
bool canSpawnCarMultiThreaded(int carCount, Car *cars[], const float spawnPositions[][3], int spawnIndex, float minDistance)
{
    const int NUM_THREADS = 10; // Adjustable number of threads
    pthread_t threads[NUM_THREADS];
    SpawnCheckArgs threadArgs[NUM_THREADS];

    // Flag to track spawn condition
    bool canSpawn = true;

    // Calculate cars per thread
    int carsPerThread = carCount / NUM_THREADS;
    int remainingCars = carCount % NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++)
    {
        threadArgs[i].carCount = carCount;
        threadArgs[i].cars = cars;
        threadArgs[i].spawnPositions = spawnPositions;
        threadArgs[i].spawnIndex = spawnIndex;
        threadArgs[i].minDistance = minDistance;
        threadArgs[i].canSpawn = &canSpawn;

        // Distribute cars evenly among threads
        threadArgs[i].startIndex = i * carsPerThread;
        threadArgs[i].endIndex = (i == NUM_THREADS - 1) ? carCount : (i + 1) * carsPerThread + (i == NUM_THREADS - 2 ? remainingCars : 0);

        // Create thread
        pthread_create(&threads[i], nullptr, checkSpawnConditionsThread, &threadArgs[i]);
    }

    // Wait for all threads to complete
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], nullptr);
    }

    // Return final spawn condition
    return canSpawn;
}

bool canSpawnCar(int carCount, Car *cars[], const float spawnPositions[][3], int spawnIndex, float minDistance)
{
    return canSpawnCarMultiThreaded(carCount, cars, spawnPositions, spawnIndex, minDistance);
}

void spawnCars(
    Car *cars[],
    CarData *carData[],
    int &carCount,
    int carsInLane[],
    const float spawnPositions[][3],
    LaneConfig laneConfigs[],
    Clock globalSpawnClock,
    Clock spawnClocks[],
    Clock car5SpawnClocks[],
    int maxCars)
{
    if (globalSpawnClock.getElapsedTime().asSeconds() >= 1.5f)
    {
        for (int i = 0; i < 8; i++)
        {
            int laneIndex = i / 2;

            // Check minimum distance from other cars
            bool canSpawn = canSpawnCar(carCount, cars, spawnPositions, i, 50);

            // Special handling for CAR6
            static Clock car6SpawnClock;
            bool shouldSpawnCar6 =
                cars[i]->isValidCar6Lane(i) &&
                car6SpawnClock.getElapsedTime().asSeconds() >= 15.0f;

            if (shouldSpawnCar6)
            {
                if (canSpawn && carCount < maxCars)
                {
                    // Clean up any null pointers in the arrays
                    for (int k = 0; k < carCount; k++)
                    {
                        if (cars[k] == nullptr)
                        {
                            for (int m = k; m < carCount - 1; m++)
                            {
                                cars[m] = cars[m + 1];
                                carData[m] = carData[m + 1];
                            }
                            carCount--;
                        }
                    }

                    // Spawn multiple CAR6 vehicles
                    int car6SpawnPositions[] = {7, 1, 5, 2};
                    for (int pos : car6SpawnPositions)
                    {
                        cars[carCount] = new Car(CAR6,
                                                 spawnPositions[pos][0],
                                                 spawnPositions[pos][1],
                                                 spawnPositions[pos][2]);

                        // Initialize car data
                        carData[carCount] = new CarData();
                        carData[carCount]->numberPlate = cars[carCount]->generateNumberPlate();
                        carData[carCount]->laneIndex = laneIndex;
                        carData[carCount]->challanStatus = false;
                        carData[carCount]->isBroken = false;
                        carData[carCount]->speedUpdateClock.restart();

                        carCount++;
                        carsInLane[laneIndex]++;
                    }

                    car6SpawnClock.restart();
                }
            }
            else if (spawnClocks[i].getElapsedTime().asSeconds() >= laneConfigs[laneIndex].spawnInterval &&
                     carsInLane[laneIndex] < 6 && carCount < maxCars)
            {
                if (canSpawn)
                {
                    // Determine car type (excluding CAR6)
                    int carType = rand() % 5; // Only choose from CAR1 to CAR5
                    bool spawnCAR5 = false;

                    // Check if we should spawn CAR5 based on lane-specific probabilities
                    if (car5SpawnClocks[i].getElapsedTime().asSeconds() >= laneConfigs[i].car5Interval)
                    {
                        if (dis(gen) < laneConfigs[i].car5Probability)
                        {
                            spawnCAR5 = true;
                            car5SpawnClocks[i].restart();
                        }
                    }

                    if (spawnCAR5)
                    {
                        carType = 4; // CAR5
                    }
                    else
                    {
                        carType = rand() % 4; // CAR1-4
                    }

                    // Create new car
                    cars[carCount] = new Car(
                        static_cast<tVehicleType>(CAR1 + carType),
                        spawnPositions[i][0],
                        spawnPositions[i][1],
                        spawnPositions[i][2]);

                    // Initialize car data
                    carData[carCount] = new CarData();
                    carData[carCount]->numberPlate = cars[carCount]->generateNumberPlate();
                    carData[carCount]->laneIndex = laneIndex;
                    carData[carCount]->challanStatus = false;
                    carData[carCount]->isBroken = false;
                    carData[carCount]->speedUpdateClock.restart();

                    carCount++;
                    carsInLane[laneIndex]++;
                    spawnClocks[i].restart();
                }
            }
        }
        globalSpawnClock.restart();
    }
}

void updateCars(RenderWindow *window, Car *cars[], TrafficLight tlights[], int carCount, int *carsInLane, SmartTraffix trafficController)
{
    // First, check if there's a CAR5 in any lane
    bool car5Present = false;
    int car5LightIndex = -1;

    for (int i = 0; i < carCount; i++)
    {
        if (cars[i]->getType() == CAR5)
        {
            // Determine which traffic light corresponds to the CAR5's direction
            float carDir = cars[i]->getDir();

            if (carDir == 270)      // East-moving lane
                car5LightIndex = 0; // North-South horizontal light
            else if (carDir == 180) // West-moving lane
                car5LightIndex = 1; // North-South horizontal light
            else if (carDir == 90)  // North-moving lane
                car5LightIndex = 2; // East-West vertical light
            else if (carDir == 0)   // South-moving lane
                car5LightIndex = 3; // East-West vertical light

            car5Present = true;
            break;
        }
    }

    // // If CAR5 is present, trigger priority handling
    // if (car5Present && car5LightIndex != -1)
    // {
    //     trafficController.handleCar5Priority(car5LightIndex);
    // }
    int newCarCount = 0;
    for (int i = 0; i < carCount; i++)
    {
        // Draw the car
        cars[i]->draw(window);

        // Determine which traffic light corresponds to the car's direction
        int correspondingLightIndex = -1;
        float carDir = cars[i]->getDir();

        // Map car's position and direction to the correct traffic light
        if (carDir == 270)               // East-moving lane
            correspondingLightIndex = 0; // North-South horizontal light
        else if (carDir == 180)          // West-moving lane
            correspondingLightIndex = 1; // North-South horizontal light
        else if (carDir == 90)           // North-moving lane
            correspondingLightIndex = 2; // East-West vertical light
        else if (carDir == 0)            // South-moving lane
            correspondingLightIndex = 3; // East-West vertical light

        // Move car based on traffic light and specific position conditions
        bool canMove = false;

        if (tlights[correspondingLightIndex].getState() == GREEN)
            canMove = true;

        if (
            (cars[i]->getX() < 600 && cars[i]->getY() == 500) ||
            (cars[i]->getX() < 600 && cars[i]->getY() == 545) ||
            (cars[i]->getX() > 370 && cars[i]->getY() == 445) ||
            (cars[i]->getX() > 370 && cars[i]->getY() == 407) ||
            (cars[i]->getX() == 510 && cars[i]->getY() > 400) ||
            (cars[i]->getX() == 545 && cars[i]->getY() > 400) ||
            (cars[i]->getX() == 405 && cars[i]->getY() < 600) ||
            (cars[i]->getX() == 445 && cars[i]->getY() < 600))
            canMove = true;

        if (canMove)
            cars[i]->move2();

        bool shouldRemoveCar = false;

        if (cars[i]->getX() > 1000 && cars[i]->getDir() == 90)
        {
            if (cars[i]->getY() == 445)
            {
                carsInLane[0]--;
                carsInLane[1]--;
                shouldRemoveCar = true;
            }
        }
        else if (cars[i]->getX() < 0 && cars[i]->getDir() == 270)
        {
            if (cars[i]->getY() == 500)
            {
                carsInLane[2]--;
                carsInLane[3]--;
                shouldRemoveCar = true;
            }
        }
        else if (cars[i]->getY() > 1000 && cars[i]->getDir() == 0)
        {
            if (cars[i]->getX() == 505)
            {
                carsInLane[4]--;
                carsInLane[5]--;
                shouldRemoveCar = true;
            }
        }
        else if (cars[i]->getY() < 0 && cars[i]->getDir() == 180)
        {
            if (cars[i]->getX() == 445)
            {
                carsInLane[6]--;
                carsInLane[7]--;
                shouldRemoveCar = true;
            }
        }

        if (!shouldRemoveCar)
        {
            // Keep the car in the array
            cars[newCarCount] = cars[i];
            newCarCount++;
        }
        else
        {
            // Delete the car and its associated data
            // delete cars[i];
        }
    }
    carCount = newCarCount;
}
