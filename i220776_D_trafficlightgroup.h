#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <chrono>
#include <thread>
#include "i220776_D_trafficlight.h"

using namespace std;
using namespace chrono;

#ifndef TRAFFICLIGHTGROUP_H
#define TRAFFICLIGHTGROUP_H

class TrafficLightGroup
{
    TrafficLight *head;       // Pointer to the head of the traffic light in the group
    TrafficLight *greenLight; // Traffic light when the state is green

    float duration; // Period
public:
    float time;                    // Current time elapsed in seconds
    vector<TrafficLight *> lights; // Store all lights in the group

    // Constructor for TrafficLightGroup class
    //  duration: TrafficLight period
    TrafficLightGroup(float duration) : head(NULL), greenLight(NULL), time(0)
    {
        // Initialize head, tail, and greenLight pointers to NULL and time to 0.
        this->duration = duration; // Initialize the duration.
    }

    // Adds traffic lights to traffic light groups
    // light: Pointer to the traffic light object to be added to the group
    void add(TrafficLight *light)
    {
        light->setNext(NULL);
        TrafficLight *tmp = head;

        if (tmp != NULL)
        {
            while (tmp->returnNext() != NULL)
                tmp = tmp->returnNext();
            tmp->setNext(light);
        }
        else
        {
            head = light;
        }

        // Also add to the vector for easier access
        lights.push_back(light);
    }
};

#endif // TRAFFICLIGHTGROUP_H
