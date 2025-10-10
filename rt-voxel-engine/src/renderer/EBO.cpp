#include "EBO.h"

EBO::EBO(const unsigned int* data, unsigned int count)
{
	glGenBuffers(1, &handle_);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle_);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(unsigned int), data, GL_STATIC_DRAW);
}

EBO::~EBO() {
	glDeleteBuffers(1, &handle_);
}

void EBO::Bind()
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle_);
}

void EBO::Unbind()
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}