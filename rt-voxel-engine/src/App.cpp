#include "App.h"
#include "config.h"
#include <string.h>

#define FAR_PLANE 9500.0f

App::App(std::string name)
	:name_(name),
#ifdef OPENGL
	chunk_manager_(RENDER_DISTANCE, PLAYER_START_POS, SEED),
#endif
#ifdef VULKAN
	chunk_manager_(RENDER_DISTANCE, PLAYER_START_POS, SEED, renderer_),
#endif
	input(InputSystem::GetInstance())
{

}

void App::Init(Window* window)
{
	window_ = window;
	window_->SetWindowTitle(name_);

	// Initialize InputSystem with keybindings configuration file
	input.Initialize(window_->GetWindowHandle(), "res/settings/input.txt");

	player_ = Player(PLAYER_START_POS, glm::vec3(-60.0f, 0.0f, 0.0f));

	camera_ = Camera(90.0f, window_->GetWidth(), window_->GetHeigth(), FAR_PLANE);
	camera_.BindEntity(&player_);

#ifdef VULKAN
	renderer_.SetWindow(window_);
#endif
	renderer_.Init(window_->GetWidth(), window_->GetHeigth());
	renderer_.SetCamera(&camera_);

	chunk_manager_.GenerateChunks();
}

void App::Tick()
{
}

void App::ProcessInput(double delta_time)
{
	if (input.IsActionJustPressed(Action::CLOSE_APP))
		window_->CloseWindow();
#ifdef OPENGL
	if (input.IsActionJustPressed(Action::WIRE_FRAME_MODE))
		renderer_.ToggleWireframeMode();
#endif

	player_.HandleInput(input, delta_time);

	input.Update();
}

void App::Update(double delta_time)
{
	player_.Update(delta_time);
	if(DYNAMIC_WORLD)
		chunk_manager_.UpdateCenter(player_.GetPosition());

	camera_.UpdateViewMatrix();
	if (window_->SizeChanged() == true && window_->NotMinimized())
	{
		camera_.UpdateProjectionMatrix(90.0f, window_->GetWidth(), window_->GetHeigth(), FAR_PLANE);
		renderer_.WindowSizeChanged(window_->GetWidth(), window_->GetHeigth());
	}
}

void App::Render()
{
	renderer_.Render(chunk_manager_);
}