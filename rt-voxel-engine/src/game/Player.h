#pragma once

#include"../input/input.h"
#include<glm/glm.hpp>

const float SPEED = 30.0f;
const glm::vec3 PLAYER_LOOK_VECTOR = glm::vec3(0.0f, 0.0f, -1.0f);

//TODO some inheritance from for example entity with position and rotation  member
class Player
{
private:
	glm::vec3 position_;
	glm::vec3 rotation_;
	glm::vec3 movement_;
	glm::vec3 rotation_change_;
	bool sprint_, crouch_;
public:
	Player(glm::vec3 position, glm::vec3 rotation);
	Player() = default;

	void HandleInput(InputSystem &input, double delta_time);
	void Update(double delta_time);
	inline glm::vec3 GetPosition() const { return position_; };
	inline glm::vec3 GetRotation() const { return rotation_; };
};

