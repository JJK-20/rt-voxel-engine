#pragma once

#include <glad/glad.h>

class EBO
{
private:
	GLuint handle_;
public:
	EBO(const unsigned int* data, unsigned int count);
	~EBO();

	void Bind();
	void Unbind();
};
