#ifdef OPENGL
#include "Renderer.h"
#include <chrono>
#include <iostream>
using namespace std::chrono;

Renderer::Renderer()
	: shader_("res/shaders/basic.vs", "res/shaders/basic.fs"),
	texture_("res/textures/blocks.png"),
	camera_(nullptr),
	wire_frame_(false)
{

}

void Renderer::Init(int width, int height)
{
	glViewport(0, 0, width, height);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	shader_.SetTexture("texture0", 0, texture_);
}

void Renderer::SetCamera(const Camera* camera)
{
	camera_ = camera;
}

void Renderer::ToggleWireframeMode()
{
	wire_frame_ = !wire_frame_;
	glPolygonMode(GL_FRONT_AND_BACK, wire_frame_ ? GL_LINE : GL_FILL);
}

void Renderer::WindowSizeChanged(int width, int height)
{
	glViewport(0, 0, width, height);
}

// TODO renderer class which take camera, and objects to draw (meshes) to render,
// it should first accumulate meshes and then draw together so we can do depth prepass or do some kind of batch rendering
void Renderer::Render(const ChunkManager& chunk_manager)
{
	static int frame_count = 0;
	static float elapsed_time = 0;
	auto start = high_resolution_clock::now();
	glClearColor(0.54f, 0.81f, 0.94f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shader_.Bind();
	shader_.SetUniformMat4f("view", camera_->GetViewMatrix());
	shader_.SetUniformMat4f("projection", camera_->GetProjectionMatrix());

	chunk_manager.Draw();
	auto stop = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>(stop - start);
	++frame_count;
	elapsed_time += duration.count() / 1000.0f;
	if (elapsed_time >= 1000.0f)
	{
		int fps = frame_count;
		std::cout << "Render time: " << duration.count() / 1000.0f << std::endl;
		std::cout << "FPS: " << fps << std::endl;

		// Reset counters
		frame_count = 0;
		elapsed_time -= 1000.0f;
	}
}
#endif