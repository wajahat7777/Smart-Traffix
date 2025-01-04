#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#ifndef TRAFFICLIGHT_H
#define TRAFFICLIGHT_H

typedef enum
{
    GREEN = 0,
    RED = 1
} tLightState; // Traffic Light state. red or green light

struct TrafficLightPosition
{
    float x, y, rotation;
};

class TrafficLight
{
    float x, y;               // Coordinates for traffic lights
    float dir;                // direction of the traffic light (determines the orientation of the traffic light on the map)
    tLightState state;        // current state of the light (either green or red). tLightState should be an enum
    TrafficLight *next;       // pointer to the next traffic light in the traffic light group
    sf::Texture redTexture;   // texture for red light
    sf::Texture greenTexture; // texture for green light
    sf::Sprite sprite;

public:
    float getX() const { return x; }
    float getY() const { return y; }
    tLightState getState() const { return state; }
    float getRotation() const { return dir; }

    TrafficLight() : x(0), y(0), dir(0), state(RED)
    {
        // Default initialization with some default values
        redTexture.loadFromFile("images/trafficlights/red.png");
        sprite.setTexture(redTexture);
        sprite.setPosition(sf::Vector2f(0, 0));
        sprite.setRotation(0);
        sprite.setOrigin(0, 0);
    }
    TrafficLight(float x, float y, float dir, tLightState state)
    {
        this->x = x;
        this->y = y;

        this->dir = dir;
        // Initialization of variables (coordiates and rotation)

        // in case of green light
        if (state == GREEN)
        {
            greenTexture.loadFromFile("images/trafficlights/green.png");
            sprite.setTexture(greenTexture);
        }

        // in case of red light
        else
        {
            redTexture.loadFromFile("images/trafficlights/red.png");
            sprite.setTexture(redTexture);
        }

        sprite.setPosition(sf::Vector2f(x, y));
        sprite.setRotation(dir);
        sprite.setOrigin(0, 0);

        this->state = state;
    }

    void setNext(TrafficLight *next)
    {
        this->next = next;
    }
    bool canMove(int x)
    {
        return state == GREEN;
    }

    TrafficLight *returnNext() { return next; }

    // Returns the position and the direction of the traffic light
    // dir: orientation
    void getPosition(float &x, float &y, float &dir)
    {
        x = this->x;
        y = this->y;
        dir = this->dir;
    }

    // New method to get position details
    TrafficLightPosition getPosition() const
    {
        return {x, y, dir};
    }

    // Draws the traffic lights to the window
    void draw(sf::RenderWindow *window) { window->draw(this->sprite); }

    // Returns current traffic light state
    tLightState getState() { return state; }

    void setState(tLightState state)
    {
        this->state = state;

        if (state == GREEN)
        {
            greenTexture.loadFromFile("images/trafficlights/green.png");
            sprite.setTexture(greenTexture);
        }
        else
        {
            redTexture.loadFromFile("images/trafficlights/red.png");
            sprite.setTexture(redTexture);
        }
    }
};

#endif // TRAFFICLIGHT_H
