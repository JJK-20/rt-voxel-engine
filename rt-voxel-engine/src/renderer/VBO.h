#pragma once

#include <glad/glad.h>

class VBO
{
private:
	GLuint handle_;
public:
	VBO(const void* data, unsigned int size);
	~VBO();

	void Bind();
	void Unbind();
};
