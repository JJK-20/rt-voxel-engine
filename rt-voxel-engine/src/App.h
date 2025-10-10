#pragma once

#include "Window.h"
#include "game/Player.h"
#include "math/Camera.h"
#include "game/chunks/ChunkManager.h"
#ifdef OPENGL
#include "renderer/Renderer.h"
#endif
#ifdef VULKAN
#include "renderer-vulkan-rt/RendererRT.h"
#endif
#include "input/input.h"
#include <glm/glm.hpp>

class Window;

class App
{
public:
	App(std::string name);
	void Init(Window* window);
	void ProcessInput(double delta_time);
	void Update(double delta_time);
	void Tick();
	void Render();
private:
	Window* window_;
	std::string name_;

	Player player_;
	Camera camera_;

#ifdef OPENGL
	Renderer renderer_;
#endif
#ifdef VULKAN
	RendererRT renderer_;
#endif

	ChunkManager chunk_manager_;

	InputSystem &input;
};
