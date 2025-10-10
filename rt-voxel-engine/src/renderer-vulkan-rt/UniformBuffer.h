#pragma once

#include <glm/glm.hpp>

struct UniformBuffer
{
	glm::mat4 view_inv;
	glm::mat4 proj_inv;
	float time;
};
