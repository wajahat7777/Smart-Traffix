#pragma once
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <iostream>
#include <cmath>
#include <random>
#include <ctime>
#include <pthread.h>
#include <queue>
#include <map>
#include "i220776_D_roadtile.h"
#include "i220776_D_trafficlightgroup.h"
#include "i220776_D_car.h"
#include <sstream>
#include <sys/wait.h>
#include <sys/time.h>
#include <string.h>

// Speed limits in km/h
const float SPEED_LIMIT_REGULAR = 60.0f; // For CAR1-4
const float SPEED_LIMIT_CAR5 = 80.0f;    // For CAR5
const float SPEED_LIMIT_CAR6 = 40.0f;    // For CAR6

using namespace std;
using namespace sf;

enum VehicleCategory
{
    REGULAR,   // CAR1-CAR4
    EMERGENCY, // CAR5
    HEAVY,     // CAR6-CAR7
    UNKNOWN
};

enum PaymentStatus
{
    PAID,
    UNPAID,
    OVERDUE
};

class ChallanRecord
{
public:
    int challanId;
    string vehicleNumber;
    VehicleCategory vehicleCategory;
    float baseAmount;
    float totalAmount;
    time_t issueDate;
    time_t dueDate;
    PaymentStatus status;
};
class ChallanNode
{
public:
    ChallanRecord challan;
    ChallanNode *next;

    ChallanNode(const ChallanRecord &record) : challan(record), next(nullptr) {}
};

class ChallanGenerator
{
private:
    ChallanNode *challanHead; // Head of the linked list of challans
    ChallanNode *challanTail; // Tail of the linked list for efficient insertion
    int nextChallanId;
    int totalChallanCount;

    const float SERVICE_CHARGE_RATE = 0.17;
    const float REGULAR_FINE = 5000.0f;
    const float HEAVY_FINE = 7000.0f;

    VehicleCategory categorizeVehicle(tVehicleType type)
    {
        if (type == CAR5)
            return EMERGENCY;
        if (type >= CAR6 && type <= CAR7)
            return HEAVY;
        if (type >= CAR1 && type <= CAR4)
            return REGULAR;
        return UNKNOWN;
    }

    float calculateFine(VehicleCategory category)
    {
        switch (category)
        {
        case REGULAR:
            return REGULAR_FINE;
        case HEAVY:
            return HEAVY_FINE;
        case EMERGENCY:
            return 0.0f;
        default:
            return REGULAR_FINE;
        }
    }

    string formatDate(time_t timestamp)
    {
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&timestamp));
        return string(buffer);
    }

public:
    ChallanGenerator() : challanHead(nullptr), challanTail(nullptr),
                         nextChallanId(1), totalChallanCount(0) {}

    ~ChallanGenerator()
    {
        // Clean up the linked list
        while (challanHead != nullptr)
        {
            ChallanNode *temp = challanHead;
            challanHead = challanHead->next;
            delete temp;
        }
    }

    ChallanRecord generateChallan(const string &vehicleNumber, tVehicleType vehicleType, float speed)
    {
        VehicleCategory category = categorizeVehicle(vehicleType);

        // Emergency vehicles are exempt
        if (category == EMERGENCY)
        {
            cout << "Emergency vehicle " << vehicleNumber << " is exempt from challans.\n";
            return {};
        }

        float baseAmount = calculateFine(category);
        float totalAmount = baseAmount * (1 + SERVICE_CHARGE_RATE);

        ChallanRecord challan;
        challan.challanId = nextChallanId++;
        challan.vehicleNumber = vehicleNumber;
        challan.vehicleCategory = category;
        challan.baseAmount = baseAmount;
        challan.totalAmount = totalAmount;
        challan.issueDate = time(nullptr);
        challan.dueDate = challan.issueDate + (3 * 24 * 60 * 60); // 3 days
        challan.status = UNPAID;

        // Create a new node and add to the linked list
        ChallanNode *newNode = new ChallanNode(challan);

        if (challanHead == nullptr)
        {
            challanHead = newNode;
            challanTail = newNode;
        }
        else
        {
            challanTail->next = newNode;
            challanTail = newNode;
        }

        totalChallanCount++;

        displayChallan(challan);
        return challan;
    }

    // Find challans for a specific vehicle number
    bool findChallansByVehicleNumber(const string &vehicleNumber, ChallanRecord *resultArray, int &resultCount, int maxResults)
    {
        resultCount = 0;
        ChallanNode *current = challanHead;

        while (current != nullptr && resultCount < maxResults)
        {
            if (current->challan.vehicleNumber == vehicleNumber)
            {
                resultArray[resultCount++] = current->challan;
            }
            current = current->next;
        }

        return resultCount > 0;
    }

    // Get total number of challans
    int getTotalChallanCount() const
    {
        return totalChallanCount;
    }

    // Iterate through all challans (for analytics or display)
    void iterateChallans(void (*callback)(const ChallanRecord &))
    {
        ChallanNode *current = challanHead;
        while (current != nullptr)
        {
            callback(current->challan);
            current = current->next;
        }
    }

    void displayChallan(const ChallanRecord &challan)
    {
        cout << "Challan Details:\n";
        cout << "ID: " << challan.challanId << "\n";
        cout << "Vehicle: " << challan.vehicleNumber << "\n";
        cout << "Category: " << getCategoryName(challan.vehicleCategory) << "\n";
        cout << "Base Fine: " << challan.baseAmount << " PKR\n";
        cout << "Total Amount: " << challan.totalAmount << " PKR\n";
        cout << "Issue Date: " << formatDate(challan.issueDate) << "\n";
        cout << "Due Date: " << formatDate(challan.dueDate) << "\n";
    }

    string getCategoryName(VehicleCategory category)
    {
        switch (category)
        {
        case REGULAR:
            return "Regular";
        case EMERGENCY:
            return "Emergency";
        case HEAVY:
            return "Heavy";
        default:
            return "Unknown";
        }
    }
};

class SmartTraffix
{
private:
    TrafficLight *trafficLights;
    int lightCount;
    Clock rotationClock;
    Clock car5PriorityClock;
    const float LIGHT_INTERVAL = 10.0f;
    const float CAR5_PRIORITY_DURATION = 5.0f;
    int currentGreenIndex;
    bool car5PriorityActive = false;
    int car5PriorityLightIndex = -1;
    vector<pid_t> processPIDs;
    int available[4];        // Number of available lanes at each light
    int maximum[5000][4];    // Maximum demand of each vehicle
    int allocation[5000][4]; // Current allocation
    int need[5000][4];       // Remaining need

    int numVehicles; 
    int numLights;   

    queue<string> activeChallans;  // Queue to manage vehicle numbers with active challans
    queue<string> speedViolations; // Queue to hold vehicle numbers violating speed
    const int SPEED_LIMIT = 60; // Speed limit in km/h

    // Analytics data (using queue instead of array)
    queue<string> vehicleCount; // Queue to store vehicle type counts
    int activeChallanCount = 0;

    void rotateTrafficLights()
    {
        int nextGreenIndex = (currentGreenIndex - 1 + lightCount) % lightCount; // Ensures it wraps around
        bool resourceAllocated = false;

        // Check for resource allocation safety
        for (int i = 0; i < numVehicles; ++i)
        {
            if (requestResource(i, nextGreenIndex))
            {
                trafficLights[currentGreenIndex].setState(RED);
                trafficLights[nextGreenIndex].setState(GREEN);
                currentGreenIndex = nextGreenIndex;
                resourceAllocated = true;
                break;
            }
        }

        // If no resource was allocated, proceed with regular rotation
        if (!resourceAllocated)
        {
            trafficLights[currentGreenIndex].setState(RED);
            trafficLights[nextGreenIndex].setState(GREEN);
            currentGreenIndex = nextGreenIndex;
        }
    }

    void detectAndResolveDeadlock()
    {
        // Detect potential deadlock (all lights are red)
        bool deadlock = true;
        for (int i = 0; i < lightCount; ++i)
        {
            if (trafficLights[i].getState() == GREEN)
            {
                deadlock = false;
                break;
            }
        }

        // Resolve deadlock by granting a green light to the least used lane
        if (deadlock)
        {
            for (int i = 0; i < lightCount; ++i)
            {
                trafficLights[i].setState(i == 0 ? GREEN : RED);
            }
        }
    }

    void createProcessForLight(int lightIndex)
    {
        pid_t pid = fork();

        if (pid < 0)
        {
            cerr << "Fork failed for light " << lightIndex << endl;
            exit(1);
        }
        else if (pid == 0)
        {
            // Child process
            while (true)
            {
                sleep(1); // Simulate work
            }
            exit(0);
        }
        else
        {
            // Parent process
            processPIDs.push_back(pid);
        }
    }

    void generateChallan(string vehicleNumber)
    {
        // Check if vehicle is not already in the active challan queue
        queue<string> tempQueue = activeChallans; // Temporarily store current state of activeChallans queue
        bool isChallanActive = false;
        while (!tempQueue.empty())
        {
            if (tempQueue.front() == vehicleNumber)
            {
                isChallanActive = true;
                break;
            }
            tempQueue.pop();
        }

        if (!isChallanActive)
        {
            activeChallans.push(vehicleNumber); // Add vehicle number to the challan queue
            activeChallanCount++;
            cout << "Challan generated for vehicle: " << vehicleNumber << endl;
        }
    }

    void updateChallanStatus()
    {
        while (!speedViolations.empty())
        {
            string vehicleNumber = speedViolations.front();
            speedViolations.pop();
            generateChallan(vehicleNumber);
        }
    }

public:
    SmartTraffix(TrafficLight *lights, int count) : trafficLights(lights),
                                                    lightCount(count),
                                                    currentGreenIndex(0)
    {
        // Initially set first light to GREEN
        trafficLights[0].setState(GREEN);
        for (int i = 1; i < lightCount; i++)
        {
            trafficLights[i].setState(RED);
        }

        // Create a process for each traffic light
        for (int i = 0; i < lightCount; i++)
        {
            createProcessForLight(i);
        }

        for (int i = 0; i < lightCount; ++i)
            available[i] = 1; // Each light initially has 1 available lane

        // Initialize allocation, maximum, and need matrices
        memset(allocation, 0, sizeof(allocation));
        memset(maximum, 0, sizeof(maximum));
        memset(need, 0, sizeof(need));
    }

    void update()
    {
        if (car5PriorityActive &&
            car5PriorityClock.getElapsedTime().asSeconds() >= CAR5_PRIORITY_DURATION)
        {
            car5PriorityActive = false;
            car5PriorityLightIndex = -1;

            trafficLights[0].setState(GREEN);
            for (int i = 1; i < lightCount; i++)
            {
                trafficLights[i].setState(RED);
            }
            rotateTrafficLights();
        }

        if (!car5PriorityActive &&
            rotationClock.getElapsedTime().asSeconds() >= LIGHT_INTERVAL)
        {
            rotateTrafficLights();
            rotationClock.restart();
        }
        detectAndResolveDeadlock();
        updateChallanStatus();
    }

    void handleCar5Priority(int lightIndex)
    {
        if (!car5PriorityActive)
        {
            car5PriorityActive = true;
            car5PriorityLightIndex = lightIndex;
            car5PriorityClock.restart();

            for (int i = 0; i < lightCount; i++)
            {
                trafficLights[i].setState(i == lightIndex ? GREEN : RED);
            }
        }
    }

    void monitorSpeed(const string &vehicleNumber, tVehicleType vehicleType, float speed, int laneIndex)
    {
        if (speed > SPEED_LIMIT)
        {
            cout << "Speed violation detected for vehicle: " << vehicleNumber
                 << " in lane " << laneIndex << endl;
            speedViolations.push(vehicleNumber);
        }
    }

    void recordVehicle(string vehicleType)
    {
        vehicleCount.push(vehicleType); // Add the vehicle type to the queue
    }

    void displayAnalytics()
    {
        cout << "Traffic Analytics:\n";
        queue<string> tempQueue = vehicleCount; // Copy vehicle count queue to a temporary one
        while (!tempQueue.empty())
        {
            cout << "Vehicle Type " << tempQueue.front() << ": 1\n"; // Displaying count as 1 for each vehicle type recorded
            tempQueue.pop();
        }
        cout << "Active Challans: " << activeChallanCount << "\n";
        // Display active challans
        tempQueue = activeChallans;
        while (!tempQueue.empty())
        {
            cout << "Vehicle " << tempQueue.front() << " has an active challan.\n";
            tempQueue.pop();
        }
    }
    bool isSafeState()
    {
        int work[4];
        bool finish[5000] = {false};

        // Initialize work with available resources
        for (int i = 0; i < numLights; ++i)
            work[i] = available[i];

        // Simulate resource allocation
        for (int i = 0; i < numVehicles; ++i)
        {
            if (!finish[i])
            {
                bool canProceed = true;
                for (int j = 0; j < numLights; ++j)
                {
                    if (need[i][j] > work[j])
                    {
                        canProceed = false;
                        break;
                    }
                }
                if (canProceed)
                {
                    for (int j = 0; j < numLights; ++j)
                        work[j] += allocation[i][j];
                    finish[i] = true;
                    i = -1; // Restart the check
                }
            }
        }

        // Check if all processes can finish
        for (int i = 0; i < numVehicles; ++i)
            if (!finish[i])
                return false;

        return true;
    }

    bool requestResource(int vehicleID, int lightIndex)
    {
        // Check if request exceeds need
        if (need[vehicleID][lightIndex] > available[lightIndex])
            return false;

        // Temporarily allocate resources
        available[lightIndex] -= need[vehicleID][lightIndex];
        allocation[vehicleID][lightIndex] += need[vehicleID][lightIndex];
        need[vehicleID][lightIndex] = 0;

        // Check for safe state
        if (!isSafeState())
        {
            // Rollback allocation if unsafe
            available[lightIndex] += need[vehicleID][lightIndex];
            allocation[vehicleID][lightIndex] -= need[vehicleID][lightIndex];
            return false;
        }

        return true;
    }

    ~SmartTraffix()
    {
        for (pid_t pid : processPIDs)
        {
            kill(pid, SIGTERM);
            waitpid(pid, nullptr, 0);
        }
    }
};

struct SpeedViolationArgs
{
    Car **cars;
    CarData **carData;
    int startIndex;
    int endIndex;
    ChallanGenerator *challanGenerator;
    SmartTraffix *trafficAnalytics;
};

void *checkSpeedViolationsThread(void *args)
{
    SpeedViolationArgs *threadArgs = static_cast<SpeedViolationArgs *>(args);

    for (int i = threadArgs->startIndex; i < threadArgs->endIndex; i++)
    {
        if (threadArgs->cars[i] == nullptr || threadArgs->carData[i] == nullptr)
            continue;

        float speedLimit;
        switch (threadArgs->cars[i]->getType())
        {
        case CAR5:
            speedLimit = 80.0f;
            break;
        case CAR6:
        case CAR7:
            speedLimit = 40.0f;
            break;
        default:
            speedLimit = 60.0f;
        }

        float currentSpeed = threadArgs->cars[i]->getSpeed();

        if (currentSpeed > speedLimit && !threadArgs->carData[i]->challanStatus)
        {
            // Generate Challan
            ChallanRecord challan = threadArgs->challanGenerator->generateChallan(
                threadArgs->carData[i]->numberPlate,
                threadArgs->cars[i]->getType(),
                currentSpeed);

            // Mark car for challan and potential deletion
            threadArgs->carData[i]->challanStatus = true;
            threadArgs->carData[i]->markedForDeletion = true;

            // Update Traffic Analytics
            threadArgs->trafficAnalytics->monitorSpeed(
                threadArgs->carData[i]->numberPlate,
                threadArgs->cars[i]->getType(),
                currentSpeed,
                threadArgs->carData[i]->laneIndex);

            // Display Analytics
           // threadArgs->trafficAnalytics->displayAnalytics();
        }
    }
    return nullptr;
}

void checkSpeedViolationsMultiThreaded(
    Car *cars[],
    CarData *carData[],
    int &carCount,
    ChallanGenerator &challanGenerator,
    SmartTraffix &trafficAnalytics)
{
    const int NUTHREADS = 4;
    pthread_t threads[NUTHREADS];
    SpeedViolationArgs threadArgs[NUTHREADS];

    int carsPerThread = carCount / NUTHREADS;

    for (int i = 0; i < NUTHREADS; i++)
    {
        threadArgs[i].cars = cars;
        threadArgs[i].carData = carData;
        threadArgs[i].startIndex = i * carsPerThread;
        threadArgs[i].endIndex = (i == NUTHREADS - 1) ? carCount : (i + 1) * carsPerThread;
        threadArgs[i].challanGenerator = &challanGenerator;
        threadArgs[i].trafficAnalytics = &trafficAnalytics;

        pthread_create(&threads[i], nullptr, checkSpeedViolationsThread, &threadArgs[i]);
    }

    for (int i = 0; i < NUTHREADS; i++)
    {
        pthread_join(threads[i], nullptr);
    }
}
class StripPayment
{
private:
    ChallanGenerator &challanGenerator;

public:
    StripPayment(ChallanGenerator &generator) : challanGenerator(generator) {}

    static void *processPaymentThread(void *args)
    {
        auto *paymentArgs = static_cast<tuple<int, string, float, ChallanGenerator *> *>(args);
        int challanId = get<0>(*paymentArgs);
        string vehicleNumber = get<1>(*paymentArgs);
        float amount = get<2>(*paymentArgs);
        ChallanGenerator *generator = get<3>(*paymentArgs);

        const int MAX_CHALLANS = 10;
        ChallanRecord challans[MAX_CHALLANS];
        int resultCount = 0;

        if (generator->findChallansByVehicleNumber(vehicleNumber, challans, resultCount, MAX_CHALLANS))
        {
            for (int i = 0; i < resultCount; i++)
            {
                if (challans[i].challanId == challanId &&
                    challans[i].status != PAID &&
                    challans[i].totalAmount == amount)
                {
                    challans[i].status = PAID;
                    cout << "Payment successful for Challan ID: " << challanId
                         << " Vehicle: " << vehicleNumber
                         << " Amount: " << amount << " PKR\n";

                    delete paymentArgs; // Clean up dynamically allocated memory
                    pthread_exit((void *)1);
                }
            }
        }

        cout << "Payment failed. Invalid challan details.\n";
        delete paymentArgs;
        pthread_exit((void *)0);
    }

    void processPayment(int challanId, const string &vehicleNumber, float amount)
    {
        pthread_t thread;
        auto *args = new tuple<int, string, float, ChallanGenerator *>(
            challanId, vehicleNumber, amount, &challanGenerator);

        pthread_create(&thread, nullptr, processPaymentThread, args);
        pthread_detach(thread); // Detach the thread for asynchronous execution
    }
};

class UserPortal
{
private:
    ChallanGenerator &challanGenerator;
    StripPayment stripPayment;

public:
    UserPortal(ChallanGenerator &generator) : challanGenerator(generator),
                                              stripPayment(generator) {}

    static void *accessChallanDetailsThread(void *args)
    {
        auto *threadArgs = static_cast<tuple<string, time_t, ChallanGenerator *> *>(args);
        string vehicleNumber = get<0>(*threadArgs);
        time_t issueDate = get<1>(*threadArgs);
        ChallanGenerator *generator = get<2>(*threadArgs);

        const int MAX_CHALLANS = 10;
        ChallanRecord challans[MAX_CHALLANS];
        int resultCount = 0;

        if (generator->findChallansByVehicleNumber(vehicleNumber, challans, resultCount, MAX_CHALLANS))
        {
            cout << "Challans for Vehicle: " << vehicleNumber << "\n";
            for (int i = 0; i < resultCount; i++)
            {
                if (issueDate == 0 || challans[i].vehicleNumber == vehicleNumber)
                {
                    cout << "Challan ID: " << challans[i].challanId << "\n";
                    cout << "Vehicle Number: " << challans[i].vehicleNumber << "\n";
                    cout << "Payment Status: "
                         << (challans[i].status == PAID ? "PAID" : (challans[i].status == OVERDUE ? "OVERDUE" : "UNPAID")) << "\n";
                    cout << "Vehicle Type: " << generator->getCategoryName(challans[i].vehicleCategory) << "\n";
                    cout << "Amount to Pay: " << challans[i].totalAmount << " PKR\n";
                    cout << "Issue Date: " << formatDate(challans[i].issueDate) << "\n";
                    cout << "Due Date: " << formatDate(challans[i].dueDate) << "\n\n";
                }
            }
        }
        else
        {
            cout << "No challans found for vehicle: " << vehicleNumber << "\n";
        }

        delete threadArgs; // Clean up dynamically allocated memory
        pthread_exit(nullptr);
    }

    void accessChallanDetails(const string &vehicleNumber, time_t issueDate)
    {
        pthread_t thread;
        auto *args = new tuple<string, time_t, ChallanGenerator *>(
            vehicleNumber, issueDate, &challanGenerator);

        pthread_create(&thread, nullptr, accessChallanDetailsThread, args);
        pthread_detach(thread);
    }

    void payChallan(int challanId, const string &vehicleNumber, float amount)
    {
        stripPayment.processPayment(challanId, vehicleNumber, amount);
    }

    static string formatDate(time_t timestamp)
    {
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&timestamp));
        return string(buffer);
    }
};
