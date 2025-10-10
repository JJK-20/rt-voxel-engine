#pragma once

#include "Window.h"
#include "UniformBuffer.h"
#include <vector>
#include <iostream>
#include <optional>
#include <vulkan/vulkan.h>
#define VMA_STATIC_VULKAN_FUNCTIONS 1
#include "vma/vk_mem_alloc.h"
#include <mutex>

#define RESOLVE_VK_DEVICE_PFN(device, funcName)												\
{																							\
	funcName = reinterpret_cast<PFN_##funcName>(vkGetDeviceProcAddr(device, "" #funcName));	\
	if (funcName == nullptr)																\
	{																						\
		const std::string name = #funcName;                                                 \
		std::cout << "Failed to resolve function " << name << std::endl;                    \
	}                                                                                       \
}

#ifdef _DEBUG
#define ASSERT_VK_RESULT(r)                                                                                 \
{                                                                                                           \
    VkResult _result_ = (r);                                                                                \
    if (_result_ != VK_SUCCESS)                                                                             \
    {                                                                                                       \
        std::cout << "Vulkan Assertion failed in Line " << __LINE__ << " with: " << _result_ << std::endl;                                                       \
    }                                                                                                       \
}
#else
#define ASSERT_VK_RESULT(r) r
#endif

struct QueueFamilyIndices
{
	std::optional<uint32_t> compute_family;		// while all below is true we need compute family for rt, still at least on my hardware it work :)
	//std::optional<uint32_t> graphics_family;	// with my knowledge, there exist no hardware, that have queue with support for 
	//std::optional<uint32_t> present_family;	// graphics operations, and need separate queue for presentation 
												// so while checking for presentation still might be required (for some exotic reasons),
												// we can assume that if queue with support for graphics operations exist
												// there also must be one which also support presentation
	bool IsComplete()
	{
		//return graphics_family.has_value() && present_family.has_value();
		return compute_family.has_value();
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;
};

struct RayTracingProperties
{
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR pipeline_properties{};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features{};
};

struct Buffer
{
	VkBuffer buffer;
	//VkDeviceMemory memory;
	VmaAllocation allocation;

	//void Free(VkDevice device, VkAllocationCallbacks* allocator)
	//{
	//	vkFreeMemory(device, memory, allocator);
	//	vkDestroyBuffer(device, buffer, allocator);
	//}
	void Free(VmaAllocator allocator)
	{
		vmaDestroyBuffer(allocator, buffer, allocation);
	}
};

struct MappedBuffer
{
	void* buffer_ptr;
	VkBuffer buffer;
	//VkDeviceMemory memory;
	VmaAllocation allocation;

	//void Free(VkDevice device, VkAllocationCallbacks* allocator)
	//{
	//	vkUnmapMemory(device, memory);
	//	vkFreeMemory(device, memory, allocator);
	//	vkDestroyBuffer(device, buffer, allocator);
	//}
	void Free(VmaAllocator allocator)
	{
		vmaUnmapMemory(allocator, allocation);
		vmaDestroyBuffer(allocator, buffer, allocation);
	}
};

struct ImageResource
{
	VkImage image;
	//VkDeviceMemory memory;
	VmaAllocation allocation;
	VkImageView image_view;
	//void Free(VkDevice device, VkAllocationCallbacks* allocator)
	//{
	//	vkDestroyImageView(device, image_view, nullptr);
	//	vkFreeMemory(device, memory, nullptr);
	//	vkDestroyImage(device, image, nullptr);
	//}
	void Free(VmaAllocator allocator, VkDevice device, VkAllocationCallbacks* allocator_callback)
	{
		vkDestroyImageView(device, image_view, allocator_callback);
		vmaDestroyImage(allocator, image, allocation);
	}
};

struct StupidFence
{
	VkFence fence;
	unsigned int* ref_count;
	StupidFence(VkFence fence, unsigned int initial_ref_count)
		:fence(fence), ref_count(new unsigned int(initial_ref_count))
	{}
	StupidFence() = default;

	operator VkFence() const
	{
		return fence;
	}

	operator const VkFence*() const
	{
		return &fence;
	}

	void Free(VkDevice device, VkAllocationCallbacks* allocator_callback)
	{
		if (--(*ref_count) == 0)
		{
			vkDestroyFence(device, fence, allocator_callback);
			delete ref_count;
		}
	}
};

struct BottomLevelAccelerationStructure
{
	VkAccelerationStructureKHR as;
	Buffer buffer;
	VkDeviceAddress handle;

	Buffer vertex_data;
	VkDeviceAddress vertex_handle;

	bool builded; //not used
	StupidFence build_status;

	inline VkBuffer& VkBuffer() { return buffer.buffer; }
	//void Free(VkDevice device, VkAllocationCallbacks* allocator)
	//{
	//	buffer.Free(device, allocator);
	//	vertex_data.Free(device, allocator);
	//	//vkDestroyAccelerationStructureKHR(device, as, nullptr);
	//}
	void Free(VmaAllocator allocator, VkDevice device, VkAllocationCallbacks* allocator_callback)
	{
		buffer.Free(allocator);
		vertex_data.Free(allocator);
		build_status.Free(device, allocator_callback);
		//vkDestroyAccelerationStructureKHR(device, as, nullptr);
	}
};

struct AccelerationStructure
{
	VkAccelerationStructureKHR as;
	Buffer buffer;
	inline VkBuffer& VkBuffer() { return buffer.buffer; }
	//void Free(VkDevice device, VkAllocationCallbacks* allocator)
	//{
	//	buffer.Free(device, allocator);
	//	//vkDestroyAccelerationStructureKHR(device, as, nullptr);
	//}
	void Free(VmaAllocator allocator)
	{
		buffer.Free(allocator);
		//vkDestroyAccelerationStructureKHR(device, as, nullptr);
	}
};

struct DeferredCleanup
{
	StupidFence ready_for_cleanup;
	VkCommandBuffer command_buffer;
	std::vector<Buffer> buffers_to_free;
};

class VulkanRTCore
{
public:
	//Init functions
	void SetWindow(Window* window) { window_ = window; };
	void CreateInstance();
	void SetupDebugMessenger();
	void CreateSurface();
	void PickPhysicalDevice();
	void CreateLogicalDevice();
	void LoadExtFuncPtr();
	void CreateSwapChain();
	void CreateSwapChainImageViews();
	void CreateDescriptorSetLayout();
	void CreateDescriptorSet();
	void CreateRayTracingPipeline();
	void CreateShaderBindingTable();
	void CreateRenderBuffer();
	void CreateTexture(const std::string& path);
	void CreateTextureSampler();
	void CreateCommandBuffer();
	void CreateUniformBuffer();
	void CreateSyncObjects();

	void Cleanup();

	//Update functions
	BottomLevelAccelerationStructure BuildBLAS(const std::vector<float>& vertices, const  std::vector<unsigned int>& indices);
	bool IsBLASBuilded(BottomLevelAccelerationStructure acceleration_structure);
	int ProcessPendingCleanups();
	void FreeBLAS(BottomLevelAccelerationStructure acceleration_structure);
	void BuildTLAS(std::vector<BottomLevelAccelerationStructure> blases);
	void UpdateDescriptorSet();
	void RecordCommandBuffer(uint32_t swap_chain_image_index);
	void Render(UniformBuffer uniform_buffer);
	void SetWindowResized(bool window_resized) { window_resized_ = window_resized; };
	void CleanupSwapChain();
	void RecreateSwapChain();

	//Vulkan extension functions pointers
	PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR = nullptr;
	PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR = nullptr;
	PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR = nullptr;
	PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR = nullptr;
	PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR = nullptr;
	PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR = nullptr;
	PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR = nullptr;
	PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR = nullptr;
private:
	//Init helpers
	std::vector<const char*> GetRequiredExtensions();
	bool CheckValidationLayerSupport();
	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info);
	bool IsDeviceSuitable(VkPhysicalDevice device);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	VkShaderModule CreateShaderModule(const std::string& file_path);

	//Update helpers
	// All memory stuff: eiter move into smth like VMA, or implement it proper way in future
	//Buffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, bool device_address = false);
	Buffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage, bool host_access, bool device_address = false);
	Buffer CreateDeviceBuffer(VkDeviceSize size, VkBufferUsageFlags usage, bool device_address = false);
	Buffer CreateDeviceBufferWithData(const void* src_data, VkDeviceSize size, VkBufferUsageFlags usage, bool device_address = false);
	MappedBuffer CreateDeviceBufferWithHostAccess(VkDeviceSize size, VkBufferUsageFlags usage, bool device_address = false);
	uint64_t GetBufferDeviceAddress(VkBuffer buffer);

	//ImageResource CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage);
	ImageResource CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, bool dedicated_allocation = false);
	ImageResource CreateImageWithData(const void* src_data, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage);

	//uint32_t FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties);
	VkCommandBuffer BeginSingleTimeCommands();
	void EndSingleTimeCommands(VkCommandBuffer command_buffer, VkFence fence = VK_NULL_HANDLE);

	void TransitionImageLayout(VkCommandBuffer command_buffer, VkImage image, VkAccessFlags src_access_mask, VkPipelineStageFlags src_stage,
		VkAccessFlags dst_access_mask, VkPipelineStageFlags dst_stage, VkImageLayout old_layout, VkImageLayout new_layout);

	const int FRAMES_IN_FLIGHT = 2;
	int frame_in_flight = 0;

	Window* window_;
	bool window_resized_ = false;

	VkInstance instance_;
	VkDebugUtilsMessengerEXT debug_messenger_;
	VkSurfaceKHR surface_;
	VkPhysicalDevice physical_device_;
	RayTracingProperties ray_tracing_properties_;
	VkDevice device_;
	VmaAllocator allocator_;
	std::mutex queue_mutex_;
	VkQueue queue_;
	VkCommandPool command_pool_;

	VkSwapchainKHR swap_chain_;
	VkFormat swap_chain_image_format_;
	VkExtent2D swap_chain_extent_;
	std::vector<VkImage> swap_chain_images_;
	std::vector<VkImageView> swap_chain_image_views_;

	VkDescriptorSetLayout descriptor_set_layout_;
	VkDescriptorPool descriptor_pool_;
	std::vector<VkDescriptorSet> descriptor_sets_;

	VkPipelineLayout pipeline_layout_;
	VkPipeline ray_tracing_pipeline_;
	uint32_t sbt_group_count_;
	Buffer sbt_ray_gen_;
	Buffer sbt_ray_miss_;
	Buffer sbt_ray_hit_;

	std::vector<ImageResource> render_buffers_;

	ImageResource texture_;
	VkSampler texture_sampler_;

	std::vector<VkCommandBuffer> command_buffers_;

	AccelerationStructure tlas_;

	std::vector<MappedBuffer> uniform_buffers_;

	Buffer vertex_buffer_addresses_;

	std::vector<VkSemaphore> image_available_semaphores_;
	std::vector<VkSemaphore> render_finished_semaphores_;
	std::vector<VkFence> in_flight_fences_;

	std::mutex cleanup_mutex_;

	std::vector<DeferredCleanup> pending_cleanups_;

	const std::vector<const char*> validation_layers_ =
	{
		"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> device_extensions_ =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
		VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
		VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		//VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,	// Promoted to Vulkan 1.2
		//VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,	// Promoted to Vulkan 1.2
		//VK_KHR_SPIRV_1_4_EXTENSION_NAME,				// Promoted to Vulkan 1.2
		//VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME	// Promoted to Vulkan 1.2
	};

#ifdef _DEBUG
	const bool enable_validation_layers_ = true;
#else
	const bool enable_validation_layers_ = false;
#endif
};
