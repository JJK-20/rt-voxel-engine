#pragma once

#include "game/Player.h" //TODO entity
#include <glm/glm.hpp>

const glm::vec3 UP_VECTOR = glm::vec3(0.0f, 1.0f, 0.0f);

class Camera
{
private:
	Player const* entity_;
	glm::mat4 view_matrix_;
	glm::mat4 projection_matrix_;
public:
	Camera(float fov, int width, int height, float far_plane);
	Camera() = default;

	void BindEntity(Player const* entity);
	void UpdateViewMatrix();
	void UpdateProjectionMatrix(float fov, int width, int height, float far_plane);

	inline glm::mat4 GetViewMatrix() const { return view_matrix_; };
	inline glm::mat4 GetProjectionMatrix() const { return projection_matrix_; };
};
