#pragma once

#include <glad/glad.h>
#include <string>

class Texture
{
private:
	GLuint handle_;
	int width_, height_;
public:
	Texture(const std::string& path);
	~Texture();

	void Bind(const GLuint slot) const;

	inline int GetWidth() const { return width_; }
	inline int GetHeigth() const { return height_; }
};