#ifdef VULKAN
#include "RendererRT.h"
#include "UniformBuffer.h"
#include <chrono>
#include <iostream>
#include <mutex>
using namespace std::chrono;

RendererRT::RendererRT()
	: vulkan_(),
	camera_(nullptr),
	rebuild_required_(true)
{

}

RendererRT::~RendererRT()
{
	vulkan_.Cleanup();
}

void RendererRT::Init(int width, int height)
{
	vulkan_.CreateInstance();
	vulkan_.SetupDebugMessenger();
	vulkan_.CreateSurface();
	vulkan_.PickPhysicalDevice();
	vulkan_.CreateLogicalDevice();
	vulkan_.LoadExtFuncPtr();
	vulkan_.CreateSwapChain();
	vulkan_.CreateSwapChainImageViews();
	vulkan_.CreateDescriptorSetLayout();
	vulkan_.CreateDescriptorSet();
	vulkan_.CreateRayTracingPipeline();
	vulkan_.CreateShaderBindingTable();
	vulkan_.CreateRenderBuffer();
	vulkan_.CreateTexture("res/textures/blocks.png");
	vulkan_.CreateTextureSampler();
	vulkan_.CreateCommandBuffer();
	vulkan_.CreateUniformBuffer();
	vulkan_.CreateSyncObjects();
}

void RendererRT::SetWindow(Window* window)
{
	vulkan_.SetWindow(window);
}

void RendererRT::SetCamera(const Camera* camera)
{
	camera_ = camera;
}

BottomLevelAccelerationStructure RendererRT::BuildBlas(const std::vector<float>& vertices, const std::vector<unsigned int>& indices)
{
	static int currently_building = 0;
	static 	std::mutex mutex;

	mutex.lock();
	if (++currently_building > 1000)
		currently_building = vulkan_.ProcessPendingCleanups();
	mutex.unlock();
	return vulkan_.BuildBLAS(vertices, indices);
}

bool RendererRT::IsBlasBuilded(BottomLevelAccelerationStructure acceleration_structure)
{
	return vulkan_.IsBLASBuilded(acceleration_structure);
}

void RendererRT::FreeBlas(BottomLevelAccelerationStructure acceleration_structure)
{
	vulkan_.FreeBLAS(acceleration_structure);
}

void RendererRT::WindowSizeChanged(int width, int height)
{
	vulkan_.SetWindowResized(true);
}

// TODO renderer class which take camera, and objects to draw (meshes) to render,
// it should first accumulate meshes and then draw together so we can do depth prepass or do some kind of batch rendering
void RendererRT::Render(const ChunkManager& chunk_manager)
{
	//TODO in RT rendered accumulating meshes instead of drawing one by one is forced, which is good for us
	//chunk_manager.Draw();
	if (rebuild_required_)
	{
		auto start = high_resolution_clock::now();
		std::vector<BottomLevelAccelerationStructure> blases = chunk_manager.GetAllBLAS();
		vulkan_.BuildTLAS(blases);
		vulkan_.UpdateDescriptorSet();
		rebuild_required_ = false;
		auto stop = high_resolution_clock::now();
		auto duration = duration_cast<milliseconds>(stop - start);
		std::cout << "Rebuild time: " << duration.count() << std::endl;
	}
	static int frame_count = 0;
	static float elapsed_time = 0;
	auto start = high_resolution_clock::now();
	UniformBuffer uniform_buffer = { glm::inverse(camera_->GetViewMatrix()), glm::inverse(camera_->GetProjectionMatrix()), glfwGetTime() };
	// adapt to vulkan coord system
	uniform_buffer.proj_inv[1][0] *= -1;
	uniform_buffer.proj_inv[1][1] *= -1;
	uniform_buffer.proj_inv[1][2] *= -1;
	uniform_buffer.proj_inv[1][3] *= -1;
	vulkan_.Render(uniform_buffer);
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