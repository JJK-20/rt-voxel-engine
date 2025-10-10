#pragma once

#include <string>

enum class Action
{
    CLOSE_APP,
    WIRE_FRAME_MODE,
    MOVE_FORWARD,
    MOVE_BACKWARD,
    MOVE_LEFT,
    MOVE_RIGHT,
    MOVE_UP,
    MOVE_DOWN,
    SPRINT,
    CROUCH,
    LOOK_UP,
    LOOK_DOWN,
    LOOK_LEFT,
    LOOK_RIGHT
    // Add more actions as needed
};

inline Action stringToAction(const std::string& actionStr) 
{
    if (actionStr == "CLOSE_APP") return Action::CLOSE_APP;
    if (actionStr == "WIRE_FRAME_MODE") return Action::WIRE_FRAME_MODE;
    if (actionStr == "MOVE_FORWARD") return Action::MOVE_FORWARD;
    if (actionStr == "MOVE_BACKWARD") return Action::MOVE_BACKWARD;
    if (actionStr == "MOVE_LEFT") return Action::MOVE_LEFT;
    if (actionStr == "MOVE_RIGHT") return Action::MOVE_RIGHT;
    if (actionStr == "MOVE_UP") return Action::MOVE_UP;
    if (actionStr == "MOVE_DOWN") return Action::MOVE_DOWN;
    if (actionStr == "SPRINT") return Action::SPRINT;
    if (actionStr == "CROUCH") return Action::CROUCH;
    if (actionStr == "LOOK_UP") return Action::LOOK_UP;
    if (actionStr == "LOOK_DOWN") return Action::LOOK_DOWN;
    if (actionStr == "LOOK_LEFT") return Action::LOOK_LEFT;
    if (actionStr == "LOOK_RIGHT") return Action::LOOK_RIGHT;
    // Add more actions as needed
    return static_cast<Action>(-1); // Invalid action
}