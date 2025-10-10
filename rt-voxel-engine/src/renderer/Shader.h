#pragma once

#include"Texture.h"
#include <glm/glm.hpp>
#include <unordered_map>

class Shader
{
private:
	unsigned int program_handle_;
	mutable std::unordered_map<std::string, int> uniform_location_cache_;
public:
	Shader(const std::string& vertex_file_path, const std::string& fragment_file_path);
	~Shader();

	void Bind() const;
	void Unbind() const;

	//TODO in future add more SetUniform variants if needed
	void SetUniform1i(const std::string& name, int value);
	void SetUniform1f(const std::string& name, float value);
	void SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3);
	void SetUniformMat4f(const std::string& name, const glm::mat4& matrix);
	void SetTexture(const std::string& name, const GLuint slot, const Texture& texture);
private:
	unsigned int CompileShader(unsigned int type, const std::string& source);
	unsigned int CreateShader(const std::string& vertex_shader, const std::string& fragment_shader);
	std::string LoadShaderSource(const std::string& file_path);

	int GetUniformLocation(const std::string& name) const;
};
