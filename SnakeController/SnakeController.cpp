#include "SnakeController.hpp"

#include <algorithm>
#include <sstream>

#include "EventT.hpp"
#include "IPort.hpp"

#include "SnakeSegments.hpp"
#include "SnakeWorld.hpp"

namespace Snake
{

ConfigurationError::ConfigurationError()
    : std::logic_error("Bad configuration of Snake::Controller.")
{}

UnexpectedEventException::UnexpectedEventException()
    : std::runtime_error("Unexpected event received!")
{}

bool checkControl(std::istream& istr, char control)
{
    char input;
    istr >> input;
    return input == control;
}

Dimension readWorldDimension(std::istream& istr)
{
    Dimension dimension;
    istr >> dimension.width >> dimension.height;
    return dimension;
}

Position readFoodPosition(std::istream& istr)
{
    if (not checkControl(istr, 'F')) {
        throw ConfigurationError();
    }

    Position position;
    istr >> position.x >> position.y;
    return position;
}

std::unique_ptr<World> readWorld(std::istream& istr)
{
    if (not checkControl(istr, 'W')) {
        throw ConfigurationError();
    }

    auto worldDimension = readWorldDimension(istr);
    auto foodPosition = readFoodPosition(istr);
    return std::make_unique<World>(worldDimension, foodPosition);
}

Direction readDirection(std::istream& istr)
{
    if (not checkControl(istr, 'S')) {
        throw ConfigurationError();
    }

    char direction;
    istr >> direction;
    switch (direction) {
        case 'U':
            return Direction_UP;
        case 'D':
            return Direction_DOWN;
        case 'L':
            return Direction_LEFT;
        case 'R':
            return Direction_RIGHT;
        default:
            throw ConfigurationError();
    }
}

Controller::Controller(IPort& displayPort, IPort& foodPort, IPort& scorePort, std::string const& initialConfiguration)
    : m_displayPort(displayPort),
      m_foodPort(foodPort),
      m_scorePort(scorePort)
{
    std::istringstream istr(initialConfiguration);

    m_world = readWorld(istr);
    m_segments = std::make_unique<Segments>(readDirection(istr));

    int length;
    istr >> length;

    while (length--) {
        Position position;
        istr >> position.x >> position.y;
        m_segments->addSegment(position);
    }

    if (length != -1) {
        throw ConfigurationError();
    }
}

Controller::~Controller()
{}

void Controller::sendPlaceNewFood(Position position)
{
    m_world->setFoodPosition(position);

    DisplayInd placeNewFood;
    placeNewFood.x = position.x;
    placeNewFood.y = position.y;
    placeNewFood.value = Cell_FOOD;

    m_displayPort.send(std::make_unique<EventT<DisplayInd>>(placeNewFood));
}

void Controller::sendClearOldFood()
{
    auto foodPosition = m_world->getFoodPosition();

    DisplayInd clearOldFood;
    clearOldFood.x = foodPosition.x;
    clearOldFood.y = foodPosition.y;
    clearOldFood.value = Cell_FREE;

    m_displayPort.send(std::make_unique<EventT<DisplayInd>>(clearOldFood));
}

void Controller::removeTailSegment()
{
    auto tail = m_segments->removeTail();

    DisplayInd clearTail;
    clearTail.x = tail.x;
    clearTail.y = tail.y;
    clearTail.value = Cell_FREE;

    m_displayPort.send(std::make_unique<EventT<DisplayInd>>(clearTail));
}

void Controller::addHeadSegment(Position position)
{
    m_segments->addHead(position);

    DisplayInd placeNewHead;
    placeNewHead.x = position.x;
    placeNewHead.y = position.y;
    placeNewHead.value = Cell_SNAKE;

    m_displayPort.send(std::make_unique<EventT<DisplayInd>>(placeNewHead));
}

void Controller::removeTailSegmentIfNotScored(Position position)
{
    if (position == m_world->getFoodPosition()) {
        m_scorePort.send(std::make_unique<EventT<ScoreInd>>());
        m_foodPort.send(std::make_unique<EventT<FoodReq>>());
    } else {
        removeTailSegment();
    }
}

void Controller::updateSegmentsIfSuccessfullMove(Position position)
{
    if (m_segments->isCollision(position) or not m_world->contains(position)) {
        m_scorePort.send(std::make_unique<EventT<LooseInd>>());
    } else {
        addHeadSegment(position);
        removeTailSegmentIfNotScored(position);
    }
}

void Controller::handleTimeoutInd()
{
    auto newHead = m_segments->nextHead();
    updateSegmentsIfSuccessfullMove(newHead);
}

void Controller::handleDirectionInd(std::unique_ptr<Event> e)
{
    m_segments->updateDirection(payload<DirectionInd>(*e).direction);
}

void Controller::updateFoodPosition(Position position, std::function<void()> clearPolicy)
{
    if (m_segments->isCollision(position)) {
        m_foodPort.send(std::make_unique<EventT<FoodReq>>());
        return;
    }

    clearPolicy();
    sendPlaceNewFood(position);
}

void Controller::handleFoodInd(std::unique_ptr<Event> e)
{
    auto newFood = payload<FoodInd>(*e);
    auto newFoodPosition = Position{newFood.x, newFood.y};

    updateFoodPosition(newFoodPosition, std::bind(&Controller::sendClearOldFood, this));
}

void Controller::handleFoodResp(std::unique_ptr<Event> e)
{
    static auto noCleanPolicy = []{};
    auto newFood = payload<FoodResp>(*e);
    auto newFoodPosition = Position{newFood.x, newFood.y};

    updateFoodPosition(newFoodPosition, noCleanPolicy);
}

void Controller::receive(std::unique_ptr<Event> e)
{
    switch (e->getMessageId()) {
        case TimeoutInd::MESSAGE_ID:
            return handleTimeoutInd();
        case DirectionInd::MESSAGE_ID:
            return handleDirectionInd(std::move(e));
        case FoodInd::MESSAGE_ID:
            return handleFoodInd(std::move(e));
        case FoodResp::MESSAGE_ID:
            return handleFoodResp(std::move(e));
        default:
            throw UnexpectedEventException();
    }
}

} // namespace Snake
