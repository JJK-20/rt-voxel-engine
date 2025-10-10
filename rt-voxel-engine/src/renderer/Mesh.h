#pragma once

#include <vector>
#include "VAO.h"

class Mesh
{
private:
	//std::vector <float> vertices_;		// for now we store all ours meshes on GPU, so vertices_ & indices_ are not needed on CPU
	//std::vector <unsigned int> indices_;	// but in the future it could be beneficial to store it on CPU so, chunks that are 
	//										// unloaded from the GPU can be loaded again without rebuilding mesh

	unsigned int indices_count_;

	VAO vao_;
	VBO vbo_;
	EBO ebo_;
public:
	Mesh(const std::vector <float>& vertices, const  std::vector <unsigned int>& indices);
	void Draw();
};

