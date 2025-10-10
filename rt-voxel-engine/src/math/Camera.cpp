#include "Camera.h"
#include <glm/gtx/rotate_vector.hpp>

Camera::Camera(float fov, int width, int height, float far_plane)
{
	projection_matrix_ = glm::perspective(glm::radians(fov), float(width) / float(height), 0.1f, far_plane);
}

void Camera::BindEntity(Player const* entity)
{
	entity_ = entity;
}

void Camera::UpdateViewMatrix()
{
	glm::vec3 look_vector = PLAYER_LOOK_VECTOR;
	look_vector = glm::rotate(look_vector, glm::radians(entity_->GetRotation().x), { 1, 0, 0 });
	look_vector = glm::rotate(look_vector, glm::radians(entity_->GetRotation().y), { 0, 1, 0 });
	look_vector = glm::rotate(look_vector, glm::radians(entity_->GetRotation().z), { 0, 0, 1 });
	view_matrix_ = glm::lookAt(entity_->GetPosition(), entity_->GetPosition() + look_vector, UP_VECTOR);
}

void Camera::UpdateProjectionMatrix(float fov, int width, int height, float far_plane)
{
	projection_matrix_ = glm::perspective(glm::radians(fov), float(width) / float(height), 0.1f, far_plane);
}
