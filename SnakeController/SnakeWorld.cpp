#include "SnakeWorld.hpp"

#include "IPort.hpp"
#include "EventT.hpp"

#include "SnakeInterface.hpp"

namespace Snake
{

World::World(IPort& foodPort, Dimension dimension, Position food)
    : m_foodPort(foodPort),
      m_foodPosition(food),
      m_dimension(dimension)
{}

void World::setFoodPosition(Position position)
{
    m_foodPosition = position;
}

Position World::getFoodPosition() const
{
    return m_foodPosition;
}

bool World::contains(Position position) const
{
    return m_dimension.isInside(position);
}

bool World::eatFood(Position position) const
{
    bool eaten = (m_foodPosition == position);
    if (eaten) {
        m_foodPort.send(std::make_unique<EventT<FoodReq>>());
    }
    return eaten;
}
} // namespace Snake
