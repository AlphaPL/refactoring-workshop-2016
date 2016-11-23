#pragma once

#include "SnakePosition.hpp"
#include "SnakeDimension.hpp"

class IPort;

namespace Snake
{

class World
{
public:
    World(IPort& foodPort, Dimension dimension, Position food);

    void setFoodPosition(Position position);
    Position getFoodPosition() const;

    bool contains(Position position) const;

    bool eatFood(Position position) const;
private:
    IPort& m_foodPort;

    Position m_foodPosition;
    Dimension m_dimension;
};

} // namespace Snake
