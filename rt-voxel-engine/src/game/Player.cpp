#include "Player.h"
#include <glm/gtx/rotate_vector.hpp>

Player::Player(glm::vec3 position, glm::vec3 rotation)
	:position_(position), rotation_(rotation), movement_(0.0), rotation_change_(0.0), sprint_(false), crouch_(false)
{
}

void Player::HandleInput(InputSystem& input, double delta_time)
{
	glm::vec3 direction(0.0);

	if (input.IsActionPressed(Action::MOVE_FORWARD))
		direction += glm::vec3(0.0, 0.0, -1.0);
	if (input.IsActionPressed(Action::MOVE_BACKWARD))
		direction += glm::vec3(0.0, 0.0, 1.0);
	if (input.IsActionPressed(Action::MOVE_LEFT))
		direction += glm::vec3(-1.0, 0.0, 0.0);
	if (input.IsActionPressed(Action::MOVE_RIGHT))
		direction += glm::vec3(1.0, 0.0, 0.0);

	if (direction != glm::vec3(0.0))
		direction = glm::normalize(direction);

	if (input.IsActionPressed(Action::MOVE_UP))
		direction += glm::vec3(0.0, 1.0, 0.0);
	if (input.IsActionPressed(Action::MOVE_DOWN))
		direction += glm::vec3(0.0, -1.0, 0.0);

	movement_ = direction;

	if (input.IsActionJustPressed(Action::SPRINT))
		sprint_ = !sprint_;
	if (input.IsActionJustPressed(Action::CROUCH))
		crouch_ = !crouch_;

	glm::vec3 rotation(0.0);

	if (input.IsActionPressed(Action::LOOK_UP))
		rotation.x = input.GetActionStrength(Action::LOOK_UP, delta_time);
	if (input.IsActionPressed(Action::LOOK_DOWN))
		rotation.x -= input.GetActionStrength(Action::LOOK_DOWN, delta_time);
	if (input.IsActionPressed(Action::LOOK_LEFT))
		rotation.y = input.GetActionStrength(Action::LOOK_LEFT, delta_time);
	if (input.IsActionPressed(Action::LOOK_RIGHT))
		rotation.y -= input.GetActionStrength(Action::LOOK_RIGHT, delta_time);

	rotation_change_ = rotation;
}

void Player::Update(double delta_time)
{
	position_ += glm::rotate(movement_, glm::radians(rotation_.y), { 0, 1, 0 }) * (float)delta_time * SPEED * (sprint_ ? 10.0f : 1.0f) * (crouch_ ? 0.1f : 1.0f);

	rotation_ += rotation_change_;

	if (rotation_.x > 89.0)
		rotation_.x = 89.0;
	else if (rotation_.x < -89.0)
		rotation_.x = -89.0;

	if (rotation_.y > 360.0)
		rotation_.y -= 360.0;
	else if (rotation_.y < 0.0)
		rotation_.y += 360.0;
}

// part of original movement code :)
// 1st big mistake, trying to optimize something which doesn't need optimization, even in worst case scenario if it would be called 6 times
// and each time need to look through list of if's for correct direction it would require at most 6*6/2=18 if's check, is it really worth optimizing?
// 2nd problem even if we decide to optimize it, this was definitely not the correct way, WTF is this xd
// i leave it here for the future self, to have laugh about this, as this is probably one of the worst and funniest idea for something

//enum class Direction
//{
//	FORWARD = -1,
//	LEFT = -2,
//	DOWN = -3,
//	BACKWARD = 1,
//	RIGHT = 2,
//	UP = 3,
//};

//void Player::Move(Direction direction)
//{
//	//TODO i love this divisions, also let's add time dependency
//	if (direction == Direction::FORWARD || direction == Direction::BACKWARD)
//	{
//		position_.x += static_cast<float>(direction) * glm::sin(glm::radians(rotation_.y)) * SPEED;
//		position_.z += static_cast<float>(direction) * glm::cos(glm::radians(rotation_.y)) * SPEED;
//	}
//	else if (direction == Direction::LEFT || direction == Direction::RIGHT)
//	{
//		position_.x += static_cast<float>(direction) / 2.0f * glm::sin(glm::radians(rotation_.y + 90)) * SPEED;
//		position_.z += static_cast<float>(direction) / 2.0f * glm::cos(glm::radians(rotation_.y + 90)) * SPEED;
//	}
//	else
//		position_.y += static_cast<float>(direction) / 3.0f * SPEED;
//}