#pragma once

#include "VBO.h"
#include "EBO.h"
#include <glad/glad.h>

class VAO
{
private:
	GLuint handle_;
public:
	VAO();
	~VAO();

	void BindBuffers(VBO& VBO, EBO& EBO);
	void LinkAttrib(GLuint layout, GLuint num_components, GLenum type, GLsizeiptr stride, void* offset);
	void Bind();
	void Unbind();
};