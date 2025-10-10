#include "VBO.h"

VBO::VBO(const void* data, unsigned int size)
{
	glGenBuffers(1, &handle_);
	glBindBuffer(GL_ARRAY_BUFFER, handle_);
	glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
}

VBO::~VBO() {
	glDeleteBuffers(1, &handle_);
}

void VBO::Bind()
{
	glBindBuffer(GL_ARRAY_BUFFER, handle_);
}

void VBO::Unbind()
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}