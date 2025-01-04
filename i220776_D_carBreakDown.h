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

// Track simulation statistics
struct SimulationStats
{
    int totalBreakdowns;
    Clock simulationTimer;
    bool hasStarted;

    SimulationStats() : totalBreakdowns(0), hasStarted(false) {}
};

// Thread argument structure for breakdown check
struct BreakdownCheckArgs
{
    Car **cars;
    CarData **carData;
    SimulationStats *stats;
    int startIndex;
    int endIndex;
    mt19937 *gen; // Pass random generator by pointer
    uniform_real_distribution<> *dis;
};

// Thread function for breakdown check
void *checkBreakdownsThread(void *args)
{
    BreakdownCheckArgs *threadArgs = static_cast<BreakdownCheckArgs *>(args);

    for (int i = threadArgs->startIndex; i < threadArgs->endIndex; i++)
    {
        if (threadArgs->cars[i] != nullptr &&
            threadArgs->carData[i] != nullptr &&
            !threadArgs->carData[i]->isBroken)
        {

            float currentBreakdownProb = 0.00001f;

            // Ensure at least minimum breakdowns
            if (threadArgs->stats->totalBreakdowns < 1)
            {
                currentBreakdownProb = max(currentBreakdownProb, 0.000005f);
            }

            // Probabilistic breakdown (use thread-safe random generation)
            if ((*threadArgs->dis)(*threadArgs->gen) < currentBreakdownProb)
            {
                threadArgs->carData[i]->isBroken = true;
                threadArgs->carData[i]->breakdownPosition = Vector2f(
                    threadArgs->cars[i]->getX(),
                    threadArgs->cars[i]->getY());

                // Atomic increment for thread safety
                __sync_fetch_and_add(&threadArgs->stats->totalBreakdowns, 1);

                cout << "Car " << threadArgs->carData[i]->numberPlate
                     << " broke down at (" << threadArgs->carData[i]->breakdownPosition.x
                     << ", " << threadArgs->carData[i]->breakdownPosition.y << ")" << endl;

                threadArgs->cars[i]->setBreakdownState(true);
            }
        }
    }

    return nullptr;
}

// Threaded breakdown check function
void checkBreakdownsMultiThreaded(Car *cars[], CarData *carData[], int carCount, SimulationStats &stats)
{
    const int NUM_THREADS = 4;
    pthread_t threads[NUM_THREADS];
    BreakdownCheckArgs threadArgs[NUM_THREADS];

    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> dis(0, 1);

    int carsPerThread = carCount / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++)
    {
        threadArgs[i].cars = cars;
        threadArgs[i].carData = carData;
        threadArgs[i].stats = &stats;
        threadArgs[i].startIndex = i * carsPerThread;
        threadArgs[i].endIndex = (i == NUM_THREADS - 1) ? carCount : (i + 1) * carsPerThread;
        threadArgs[i].gen = &gen;
        threadArgs[i].dis = &dis;

        pthread_create(&threads[i], nullptr, checkBreakdownsThread, &threadArgs[i]);
    }

    // Wait for all threads to complete
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], nullptr);
    }
}

void spawnRescueVehiclesForBrokenDownCars(Car *cars[], CarData *carData[], int &carCount, int maxCars, int *carsInLane)
{
    for (int i = 0; i < carCount; i++)
    {
        if (carData[i]->isBroken && !carData[i]->hasSpawnedRescueVehicle)
        {
            // Spawn rescue vehicle behind the broken car
            float spawnX = carData[i]->breakdownPosition.x;
            float spawnY = carData[i]->breakdownPosition.y;
            float direction = cars[i]->getDir();

            // Adjust spawn position based on car's direction
            switch (static_cast<int>(direction))
            {
            case 0: // South-moving
                spawnY += 50;
                break;
            case 90: // East-moving
                spawnX += 50;
                break;
            case 180: // West-moving
                spawnY -= 50;
                break;
            case 270: // North-moving
                spawnX -= 50;
                break;
            }

            // Check if we have space to spawn a new vehicle
            if (carCount < maxCars)
            {
                cars[carCount] = new Car(CAR7, spawnX, spawnY, direction);

                // Initialize car data for the new CAR7 (rescue vehicle)
                carData[carCount] = new CarData();
                carData[carCount]->numberPlate = cars[carCount]->generateNumberPlate();
                carData[carCount]->laneIndex = carData[i]->laneIndex;
                carData[carCount]->challanStatus = false;
                carData[carCount]->isBroken = false;
                carData[carCount]->speedUpdateClock.restart();

                // Set the speed of the rescue vehicle to match the broken-down car
                float currentSpeed = cars[i]->getSpeed();
                cars[carCount]->setSpeed(currentSpeed);

                carCount++;
                carsInLane[carData[i]->laneIndex]++;

                // Mark that a rescue vehicle has been spawned for this broken down car
                carData[i]->hasSpawnedRescueVehicle = true;
            }
        }
    }
}