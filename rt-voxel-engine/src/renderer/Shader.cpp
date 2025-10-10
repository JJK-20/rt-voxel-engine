#include "shader.h"

#include <glad/glad.h>
#include <iostream>
#include <fstream>
#include <sstream>

Shader::Shader(const std::string& vertex_file_path, const std::string& fragment_file_path)
{
	std::string vertex_source = LoadShaderSource(vertex_file_path);
	std::string fragment_source = LoadShaderSource(fragment_file_path);

	program_handle_ = CreateShader(vertex_source, fragment_source);
}

Shader::~Shader()
{
	glDeleteProgram(program_handle_);
}

unsigned int Shader::CompileShader(unsigned int type, const std::string& source)
{
	unsigned int id = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(id, 1, &src, nullptr);
	glCompileShader(id);

	int result;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);
	if (result == GL_FALSE)
	{
		int length;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
		char* message = (char*)alloca(length * sizeof(char));
		glGetShaderInfoLog(id, length, &length, message);
		std::cout << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader!" << std::endl;
		std::cout << message << std::endl;
		glDeleteShader(id);
		return 0;
	}

	return id;
}

unsigned int Shader::CreateShader(const std::string& vertexShader, const std::string& fragmentShader)
{
	unsigned int program = glCreateProgram();
	unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
	unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	//glValidateProgram(program);
	int result;
	glGetProgramiv(program, GL_LINK_STATUS, &result);
	if (result == GL_FALSE)
	{
		int length;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
		char* message = (char*)alloca(length * sizeof(char));
		glGetProgramInfoLog(program, length, &length, message);
		std::cout << "Failed to link " << " shader!" << std::endl;
		std::cout << message << std::endl;
		glDeleteShader(vs);
		glDeleteShader(fs);
		glDeleteProgram(program);
		return 0;
	}

	glDeleteShader(vs);
	glDeleteShader(fs);

	return program;
}

std::string Shader::LoadShaderSource(const std::string& file_path)
{
	std::ifstream file_stream;
	std::stringstream stream;

	file_stream.open(file_path);
	stream << file_stream.rdbuf();
	file_stream.close();
	std::string source = stream.str();
	return source;
}

void Shader::Bind() const
{
	glUseProgram(program_handle_);
}

void Shader::Unbind() const
{
	glUseProgram(0);
}

void Shader::SetUniform1i(const std::string& name, int value)
{
	glUniform1i(GetUniformLocation(name), value);
}

void Shader::SetUniform1f(const std::string& name, float value)
{
	glUniform1f(GetUniformLocation(name), value);
}

void Shader::SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3)
{
	glUniform4f(GetUniformLocation(name), v0, v1, v2, v3);
}

void Shader::SetUniformMat4f(const std::string& name, const glm::mat4& matrix)
{
	glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &matrix[0][0]);
}

void Shader::SetTexture(const std::string& name, const GLuint slot, const Texture& texture)
{
	texture.Bind(slot);
	SetUniform1i(name, slot);
}

int Shader::GetUniformLocation(const std::string& name) const
{
	if (uniform_location_cache_.find(name) != uniform_location_cache_.end())
		return uniform_location_cache_[name];

	int location = glGetUniformLocation(program_handle_, name.c_str());
	if (location == -1)
		std::cout << "Warning: uniform '" << name << "' doesn't exist!" << std::endl;

	uniform_location_cache_[name] = location;
	return location;
}
