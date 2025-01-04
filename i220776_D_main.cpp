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
#include "i220776_D_carBreakDown.h"
#include "i220776_D_spawnCars.h"

#define WIDTH 1200
#define HEIGHT 1200
#define MIN_CAR_DISTANCE 50
#define NUM_CAR_TYPES 6
#define SIMULATION_TIME 300.0f // 5 minutes in seconds



using namespace std;
using namespace sf;

float INITIAL_LANE_SPEEDS[] = {
    6.0f, // North lane
    6.0f, // South lane
    6.0f, // East lane
    6.0f  // West lane
};

// Function to check if current time is during peak hours
bool isPeakHour()
{
    time_t now = time(0);
    tm *ltm = localtime(&now);

    // Morning peak hours: 7:00 AM to 9:30 AM
    if ((ltm->tm_hour == 7 && ltm->tm_min >= 0) ||
        (ltm->tm_hour == 8) ||
        (ltm->tm_hour == 9 && ltm->tm_min <= 30))
    {
        return true;
    }

    // Evening peak hours: 4:30 PM to 8:30 PM
    if ((ltm->tm_hour == 16 && ltm->tm_min >= 30) ||
        (ltm->tm_hour == 17) ||
        (ltm->tm_hour == 18) ||
        (ltm->tm_hour == 20 && ltm->tm_min <= 30))
    {
        return true;
    }

    return false;
}



int main()
{
    RenderWindow window(VideoMode(1000, 1000), "Traffic Simulator");

    window.setPosition(Vector2i(20, 20));
    SimulationStats stats;
    stats.simulationTimer.restart();
    stats.hasStarted = true;
   

    LaneConfig laneConfigs[] = {
        // North lanes (lanes 0-1)
        {INITIAL_LANE_SPEEDS[0], Clock(), 1.0f, 0.20f, 15.0f}, // 1 vehicle/sec, 20% CAR5 every 15s
        {INITIAL_LANE_SPEEDS[1], Clock(), 1.0f, 0.20f, 15.0f}, // 1 vehicle/sec, 20% CAR5 every 15s

        // South lanes (lanes 2-3)
        {INITIAL_LANE_SPEEDS[2], Clock(), 2.0f, 0.05f, 15.0f}, // 1 vehicle/2sec, 5% CAR5
        {INITIAL_LANE_SPEEDS[3], Clock(), 2.0f, 0.05f, 15.0f}, // 1 vehicle/2sec, 5% CAR5

        // East lanes (lanes 4-5)
        {INITIAL_LANE_SPEEDS[4], Clock(), 1.5f, 0.10f, 20.0f}, // 1 vehicle/1.5sec, 10% CAR5 every 20s
        {INITIAL_LANE_SPEEDS[5], Clock(), 1.5f, 0.10f, 20.0f}, // 1 vehicle/1.5sec, 10% CAR5 every 20s

        // West lanes (lanes 6-7)
        {INITIAL_LANE_SPEEDS[6], Clock(), 2.0f, 0.30f, 15.0f}, // 1 vehicle/2sec, 30% CAR5
        {INITIAL_LANE_SPEEDS[7], Clock(), 2.0f, 0.30f, 15.0f}  // 1 vehicle/2sec, 30% CAR5
    };

    RoadTile roadtiles[] = {
        {VER, 0, 2},
        {VER, 1, 2},
        {HOR, 2, 0},
        {HOR, 2, 1},
        {CROSS, 2, 2},
        {HOR, 2, 3},
        {HOR, 2, 4},
        {HOR, 2, 5},
        {VER, 4, 2},
        {VER, 3, 2},
        {VER, 5, 2},
    };

    TrafficLight tlights[] = {
        {535, 500, 180, GREEN},
        {510, 500, 90, RED},
        {430, 500, 180, RED},
        {500, 400, 90, GREEN},
    };
    bool move = true;
    SmartTraffix trafficController(tlights, 4);

    struct LaneConfig
    {
        float spawnInterval;
        float car5Probability;
        float car5Interval;
    };

    const int maxCars = 50000;
    Car *cars[maxCars] = {nullptr};
    CarData *carData[maxCars] = {nullptr};
    int carCount = 0;

    // Track number of cars in each lane (8 lanes in total)
    int carsInLane[8] = {};

    // Define different speeds for each lane (pixels per move)
    float laneSpeeds[8] = {1.0f, 1.0, 2.0, 2.0f, 1.5f, 1.5, 2.0f, 2.0};

    // Initialize clocks for spawning
    Clock globalSpawnClock; // Add this new clock for controlling overall spawn timing
    Clock spawnClocks[8];
    Clock specialSpawnClocks[8];
    Clock car5SpawnClocks[8];
    Clock speedtimer;
    speedtimer.restart();
    // Define fixed lane positions for spawning cars
    float spawnPositions[][3] = {
        {510, -80, 0},    // North-moving lane
        {545, -80, 0},    // North-moving lane
        {405, 1070, 180}, // South-moving lane
        {445, 1070, 180}, // South-moving lane
        {-80, 445, 90},   // East-moving lane
        {-80, 407, 90},   // East-moving lane
        {1070, 500, 270}, // West-moving lane
        {1070, 545, 270}  // West-moving lane
    };
    ChallanGenerator challanGenerator;
    UserPortal userPortal(challanGenerator);

    int counter = 0;
    bool isPaused = false; // Flag to control pause state

    while (window.isOpen())
    {

        Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
                window.close();
            if (event.type == Event::KeyPressed && event.key.code == Keyboard::P)
            {
                isPaused = !isPaused;

                if (isPaused)
                {
                   
                    // Loop to allow multiple payments
                    char payChoice;
                    do
                    {
                        cout << "Do you want to pay a challan? (y/n): ";
                        cin >> payChoice;

                        if (tolower(payChoice) == 'y')
                        {
                            string vehicleNumber;
                            cout << "Enter Vehicle Number: ";
                            cin >> vehicleNumber; 

                            string issueDateStr;
                            time_t issueDate = 0;
                            cout << "Enter Issue Date (YYYY-MM-DD) or press Enter to skip: ";
                            cin.ignore(); 
                            getline(cin, issueDateStr);

                            if (!issueDateStr.empty())
                            {
                                struct tm tm = {};
                                if (strptime(issueDateStr.c_str(), "%Y-%m-%d", &tm) != nullptr)
                                {
                                    issueDate = mktime(&tm);
                                }
                                else
                                {
                                    cout << "Invalid date format. Showing all challans.\n";
                                }
                            }

                            userPortal.accessChallanDetails(vehicleNumber, issueDate);
                            
                            int challanId;
                            float amount;
                            cout << "Enter Challan ID to pay: ";
                            cin >> challanId;
                            cout << "Enter Amount to Pay: ";
                            cin >> amount;
                            userPortal.payChallan(challanId, vehicleNumber, amount);
                        }
                        else if (tolower(payChoice) != 'n')
                        {
                            cout << "Invalid choice. Please enter 'y' or 'n'.\n";
                        }
                    } while (tolower(payChoice) != 'n'); 
                }
                else
                {
                    window.setTitle("Traffic Simulator");
                }
            }
        }

        if (!isPaused)
        {
            spawnCars(cars, carData, carCount, carsInLane, spawnPositions, laneConfigs, globalSpawnClock, spawnClocks, car5SpawnClocks, maxCars);
            trafficController.update();

            if (speedtimer.getElapsedTime().asSeconds() >= 1.0f)
            {
                counter++;
                speedtimer.restart();
                for (int temp = 0; temp < carCount; temp++)
                {
                    if ((rand() % 2) == temp % 2)
                    {
                        cars[temp]->setSpeed((INITIAL_LANE_SPEEDS[temp] + counter) * KMH_TO_PIXELS);
                    }
                }
            }

            checkBreakdownsMultiThreaded(cars, carData, carCount, stats);
            spawnRescueVehiclesForBrokenDownCars(cars, carData, carCount, maxCars, carsInLane);
            window.clear(Color::White);

            // Draw road tiles and traffic lights
            for (auto &tile : roadtiles)
            {
                tile.draw(&window);
            }
            for (auto &light : tlights)
            {
                light.draw(&window);
            }

            // Move and draw cars, with removal logic
            int removeCount = 0;
            int removeIndices[maxCars];

            updateCars(&window, cars, tlights, carCount, carsInLane, trafficController);

            // Check speed violations
            checkSpeedViolationsMultiThreaded(cars, carData, carCount, challanGenerator, trafficController);
            window.display();
        }
        else
        {
            // Keep window displaying as paused until resumed
            window.display();
        }

        // Small delay to avoid maxing out CPU usage
        sleep(sf::seconds(0.01));
    }

    // Clean up memory
    for (int i = 0; i < carCount; i++)
    {
        if (cars[i] != nullptr)
        {
            delete cars[i];
        }
    }

    return 0;
}
