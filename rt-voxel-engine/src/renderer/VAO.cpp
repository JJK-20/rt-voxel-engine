#include "VAO.h"

VAO::VAO()
{
	glGenVertexArrays(1, &handle_);
}

VAO::~VAO()
{
	glDeleteVertexArrays(1, &handle_);
}

void VAO::BindBuffers(VBO& VBO, EBO& EBO)
{
	Bind();
	VBO.Bind();
	EBO.Bind();
	Unbind();
}

void VAO::LinkAttrib(GLuint index, GLuint num_components, GLenum type, GLsizeiptr stride, void* offset)
{
	Bind();
	glVertexAttribPointer(index, num_components, type, GL_FALSE, stride, offset);
	glEnableVertexAttribArray(index);
	Unbind();
}

void VAO::Bind()
{
	glBindVertexArray(handle_);
}

void VAO::Unbind()
{
	glBindVertexArray(0);
}