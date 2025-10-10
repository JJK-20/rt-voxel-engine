#include "Window.h"
#include "App.h"
#include <iostream>

Window::Window(const int width, const int height)
	: width_(width), height_(height), size_changed_(false),
	window_title_("Application")
{
	glfwInit();
#ifdef OPENGL
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
#ifdef VULKAN
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif

	window_handle_ = glfwCreateWindow(width_, height_, window_title_.data(), NULL, NULL);
	if (window_handle_ == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
#ifdef OPENGL
	glfwMakeContextCurrent(window_handle_);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
#endif

	glfwSetWindowUserPointer(window_handle_, this);
	auto size_callback = [](GLFWwindow* window, int width, int height)
	{
		Window* instance = static_cast<Window*>(glfwGetWindowUserPointer(window));

		if (instance->width_ != width || instance->height_ != height)
			instance->size_changed_ = true;

		instance->width_ = width;
		instance->height_ = height;
	};

	glfwSetFramebufferSizeCallback(window_handle_, size_callback);
}

Window::~Window()
{
	glfwDestroyWindow(window_handle_);
	glfwTerminate();
}

void Window::Run(App& app)
{
	this->app_ = &app;
	this->app_->Init(this);
	glfwSetWindowTitle(window_handle_, window_title_.data());

	while (!glfwWindowShouldClose(window_handle_))
	{
		static double last_frame = glfwGetTime();

		double current_frame = glfwGetTime();
		double delta_time = current_frame - last_frame;
		last_frame = current_frame;

		app_->ProcessInput(delta_time);
		app_->Update(delta_time);
		app_->Render();

		glfwSwapBuffers(window_handle_);
		glfwPollEvents();
	}
}

bool Window::SizeChanged()
{
	bool size_changed = size_changed_;
	size_changed_ = false;
	return size_changed;
}

void Window::CloseWindow()
{
	glfwSetWindowShouldClose(window_handle_, true);
}

#ifdef VULKAN
const char** Window::GetRequiredInstanceExtensions(unsigned int* glfwExtensionCount) const
{
	return glfwGetRequiredInstanceExtensions(glfwExtensionCount);
};
VkResult Window::CreateWindowSurface(VkInstance* instance, const VkAllocationCallbacks* allocation_callback, VkSurfaceKHR* surface)
{
	return glfwCreateWindowSurface(*instance, window_handle_, allocation_callback, surface);
};
#endif