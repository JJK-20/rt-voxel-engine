#include "Mesh.h"

Mesh::Mesh(const std::vector<float>& vertices, const  std::vector<unsigned int>& indices)
	//: vertices_(vertices), indices_(indices), vao_(),
	//vbo_(vertices_.data(), vertices_.size() * sizeof(float)),
	//ebo_(indices_.data(), indices_.size())
	: indices_count_(indices.size()), vao_(),
	vbo_(vertices.data(), vertices.size() * sizeof(float)),
	ebo_(indices.data(), indices.size())
{
	vao_.BindBuffers(vbo_, ebo_);

	//TODO for now vertex layout is hardcoded same goes for types
	vao_.LinkAttrib(0, 3, GL_FLOAT, 6 * sizeof(float), (void*)0);
	vao_.LinkAttrib(1, 2, GL_FLOAT, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	vao_.LinkAttrib(2, 1, GL_FLOAT, 6 * sizeof(float), (void*)(5 * sizeof(float)));	//temporary simple lighting
}

void Mesh::Draw()
{
	vao_.Bind();
	glDrawElements(GL_TRIANGLES, indices_count_, GL_UNSIGNED_INT, 0);
	vao_.Unbind();
}
