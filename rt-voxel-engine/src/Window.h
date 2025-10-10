#pragma once

#ifdef OPENGL
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#endif

#ifdef VULKAN
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>
#include <string>

class App;

class Window
{
public:
	Window() : Window(1080, 720) {}
	Window(const int width, const int height);
	~Window();
	void Run(App& app);

	void SetWindowTitle(std::string& title) { window_title_ = title; };
	GLFWwindow* GetWindowHandle() const { return window_handle_; }
	int GetWidth() const { return width_; }
	int GetHeigth() const { return height_; }
	bool SizeChanged();
	bool NotMinimized() const { return width_ != 0 && height_ != 0; }
	void CloseWindow();
#ifdef VULKAN
	const char** GetRequiredInstanceExtensions(unsigned int* glfwExtensionCount) const;
	VkResult CreateWindowSurface(VkInstance* instance, const VkAllocationCallbacks* allocation_callback, VkSurfaceKHR* surface);
#endif
private:
	GLFWwindow* window_handle_;
	int width_, height_;
	bool size_changed_;
	std::string window_title_;
	App* app_;	// TODO: make some abstract app class (interface)
};