#include "i220776_D_car.h"
#include <iostream>

// Constructor definition
Car::Car(tVehicleType type, float x, float y, float dir) : Vehicle(x, y, dir), vehicleType(type)
{
    string texturePath;
    switch (type)
    {
    case CAR1:
        texturePath = "images/vehicles/car6.png";
        break;
    case CAR2:
        texturePath = "images/vehicles/car2.png";
        break;
    case CAR3:
        texturePath = "images/vehicles/car3.png";
        break;
    case CAR4:
        texturePath = "images/vehicles/car4.png";
        break;
    case CAR5:
        texturePath = "images/vehicles/ambulance.png";
        break;
    case CAR6:
        texturePath = "images/vehicles/bus.png";
        break;
    case CAR7:
        texturePath = "images/vehicles/car7.png";
        break;
    }

    if (!texture.loadFromFile(texturePath))
    {
        cerr << "Error: Failed to load texture for " << texturePath << "\n";
    }
    sprite.setTexture(texture);
    sprite.setOrigin(sprite.getGlobalBounds().width / 6, sprite.getGlobalBounds().height / 6);
    sprite.setPosition(x, y);
}

void Car::move2()
{
    // Movement speed can be adjusted here.
    // Move based on direction (dir): 0 = down, 90 = right, 180 = up, 270 = left
    if (dir == 0)
    {
        y += speed * 0.05; // Moving down
    }
    else if (dir == 90)
    {
        x += speed * 0.05; // Moving right
    }
    else if (dir == 180)
    {
        y -= speed * 0.05; // Moving up
    }
    else if (dir == 270)
    {
        x -= speed * 0.05; // Moving left
    }

    // Update the sprite position after movement
    sprite.setPosition(x, y);

    if (dir == 90)
        sprite.setRotation(dir * 4);
    else if (dir == 270)
        sprite.setRotation(dir * 2);
    else if (dir == 180)
        sprite.setRotation(270);
    else if (dir == 0)
        sprite.setRotation(90);
}

// draw method definition
void Car::draw(sf::RenderWindow *window)
{
    window->draw(sprite);
}
