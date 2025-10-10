#ifdef VULKAN
#include "VulkanRTCore.h"
#include <stb/stb_image.h>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <algorithm>
#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"

void VulkanRTCore::CreateInstance()
{
	// 1. Application Info
	VkApplicationInfo app_info{};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Vulkan Ray Tracing Renderer";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "No Engine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_2; // Vulkan 1.2 is minimum for ray tracing

	// 2. Required Extensions
	auto extensions = GetRequiredExtensions();

	// 3. Validation Layers
	VkInstanceCreateInfo instance_info{};
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pApplicationInfo = &app_info;
	instance_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instance_info.ppEnabledExtensionNames = extensions.data();

	// Check for validation layer support
	if (enable_validation_layers_ && !CheckValidationLayerSupport())
		throw std::runtime_error("Validation layers requested, but not available!");

	// Initialize debug messenger info if validation layers are enabled
	VkDebugUtilsMessengerCreateInfoEXT debug_info{};
	if (enable_validation_layers_)
	{
		instance_info.enabledLayerCount = static_cast<uint32_t>(validation_layers_.size());
		instance_info.ppEnabledLayerNames = validation_layers_.data();

		PopulateDebugMessengerCreateInfo(debug_info);

		// Link debug messenger to the Vulkan instance creation
		instance_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_info;
	}
	else
	{
		instance_info.enabledLayerCount = 0;
		instance_info.pNext = nullptr;
	}

	// 4. Create Vulkan Instance
	ASSERT_VK_RESULT(vkCreateInstance(&instance_info, nullptr, &instance_));
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* create_info, const VkAllocationCallbacks* allocator, VkDebugUtilsMessengerEXT* messenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
		return func(instance, create_info, allocator, messenger);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* allocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
		func(instance, messenger, allocator);
}

void VulkanRTCore::SetupDebugMessenger()
{
	if (!enable_validation_layers_)
		return;

	VkDebugUtilsMessengerCreateInfoEXT create_info;
	PopulateDebugMessengerCreateInfo(create_info);

	ASSERT_VK_RESULT(CreateDebugUtilsMessengerEXT(instance_, &create_info, nullptr, &debug_messenger_));
}

void VulkanRTCore::CreateSurface()
{
	ASSERT_VK_RESULT(window_->CreateWindowSurface(&instance_, nullptr, &surface_));
}

void VulkanRTCore::PickPhysicalDevice()
{
	uint32_t device_count = 0;
	ASSERT_VK_RESULT(vkEnumeratePhysicalDevices(instance_, &device_count, nullptr));

	if (device_count == 0)
		throw std::runtime_error("Failed to find GPUs with Vulkan support!");

	std::vector<VkPhysicalDevice> devices(device_count);
	ASSERT_VK_RESULT(vkEnumeratePhysicalDevices(instance_, &device_count, devices.data()));

	// Evaluate each device and pick the best one
	for (const auto& device : devices)
	{
		if (IsDeviceSuitable(device))
		{
			physical_device_ = device;
			break;
		}
	}

	if (physical_device_ == VK_NULL_HANDLE)
		throw std::runtime_error("No ray tracing compatible GPU found");
	else
	{
		VkPhysicalDeviceProperties device_properties;
		vkGetPhysicalDeviceProperties(physical_device_, &device_properties);
		std::cout << "GPU: " << device_properties.deviceName << std::endl;

		ray_tracing_properties_.pipeline_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
		VkPhysicalDeviceProperties2 device_properties_2 = {};
		device_properties_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		device_properties_2.pNext = &ray_tracing_properties_.pipeline_properties;
		vkGetPhysicalDeviceProperties2(physical_device_, &device_properties_2);

		ray_tracing_properties_.acceleration_structure_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		VkPhysicalDeviceFeatures2 device_features_2 = {};
		device_features_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		device_features_2.pNext = &ray_tracing_properties_.acceleration_structure_features;
		vkGetPhysicalDeviceFeatures2(physical_device_, &device_features_2);
	}
}

void VulkanRTCore::CreateLogicalDevice()
{
	QueueFamilyIndices indices = FindQueueFamilies(physical_device_);
	uint32_t queue_index = indices.compute_family.value();

	float queue_priority = 1.0f;
	VkDeviceQueueCreateInfo queue_info{};
	queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_info.queueFamilyIndex = queue_index;
	queue_info.queueCount = 1;
	queue_info.pQueuePriorities = &queue_priority;

	VkPhysicalDeviceFeatures device_features{};

	device_features.shaderInt64 = VK_TRUE;
	device_features.samplerAnisotropy = VK_TRUE;

	VkPhysicalDeviceBufferDeviceAddressFeatures device_buffer_device_address_features = {};
	device_buffer_device_address_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
	device_buffer_device_address_features.bufferDeviceAddress = VK_TRUE;
	device_buffer_device_address_features.pNext = nullptr;

	VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features{};
	acceleration_structure_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	acceleration_structure_features.accelerationStructure = VK_TRUE;
	acceleration_structure_features.pNext = &device_buffer_device_address_features;

	VkPhysicalDeviceRayTracingPipelineFeaturesKHR ray_tracing_pipeline_features{};
	ray_tracing_pipeline_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
	ray_tracing_pipeline_features.rayTracingPipeline = VK_TRUE;
	ray_tracing_pipeline_features.pNext = &acceleration_structure_features;

	VkDeviceCreateInfo device_info{};
	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.queueCreateInfoCount = 1;
	device_info.pQueueCreateInfos = &queue_info;
	device_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions_.size());
	device_info.ppEnabledExtensionNames = device_extensions_.data();
	device_info.pEnabledFeatures = &device_features;
	device_info.pNext = &ray_tracing_pipeline_features;

	ASSERT_VK_RESULT(vkCreateDevice(physical_device_, &device_info, nullptr, &device_));

	VmaAllocatorCreateInfo allocator_create_info = {};
	allocator_create_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_2;
	allocator_create_info.physicalDevice = physical_device_;
	allocator_create_info.device = device_;
	allocator_create_info.instance = instance_;
	vmaCreateAllocator(&allocator_create_info, &allocator_);

	vkGetDeviceQueue(device_, queue_index, 0, &queue_);

	VkCommandPoolCreateInfo command_pool_info{};
	command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	command_pool_info.queueFamilyIndex = queue_index;
	ASSERT_VK_RESULT(vkCreateCommandPool(device_, &command_pool_info, nullptr, &command_pool_));
}

void VulkanRTCore::LoadExtFuncPtr()
{
	RESOLVE_VK_DEVICE_PFN(device_, vkCreateAccelerationStructureKHR);
	RESOLVE_VK_DEVICE_PFN(device_, vkGetAccelerationStructureBuildSizesKHR);
	RESOLVE_VK_DEVICE_PFN(device_, vkCmdBuildAccelerationStructuresKHR);
	RESOLVE_VK_DEVICE_PFN(device_, vkGetAccelerationStructureDeviceAddressKHR);
	RESOLVE_VK_DEVICE_PFN(device_, vkDestroyAccelerationStructureKHR);
	RESOLVE_VK_DEVICE_PFN(device_, vkCreateRayTracingPipelinesKHR);
	RESOLVE_VK_DEVICE_PFN(device_, vkGetRayTracingShaderGroupHandlesKHR);
	RESOLVE_VK_DEVICE_PFN(device_, vkCmdTraceRaysKHR);
}

void VulkanRTCore::CreateSwapChain()
{
	SwapChainSupportDetails swap_chain_support = QuerySwapChainSupport(physical_device_);

	VkSurfaceFormatKHR surface_format = ChooseSwapSurfaceFormat(swap_chain_support.formats);
	VkPresentModeKHR present_mode = ChooseSwapPresentMode(swap_chain_support.present_modes);
	VkExtent2D extent = ChooseSwapExtent(swap_chain_support.capabilities);

	uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
	if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount)
		image_count = swap_chain_support.capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = surface_;
	create_info.minImageCount = image_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	//{	// in case of multiple queues
	//	create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
	//	create_info.queueFamilyIndexCount = 2;
	//	create_info.pQueueFamilyIndices = queueFamilyIndices;
	//}
	create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.preTransform = swap_chain_support.capabilities.currentTransform;	//VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	ASSERT_VK_RESULT(vkCreateSwapchainKHR(device_, &create_info, nullptr, &swap_chain_));

	ASSERT_VK_RESULT(vkGetSwapchainImagesKHR(device_, swap_chain_, &image_count, nullptr));
	swap_chain_images_.resize(image_count);
	ASSERT_VK_RESULT(vkGetSwapchainImagesKHR(device_, swap_chain_, &image_count, swap_chain_images_.data()));

	swap_chain_image_format_ = surface_format.format;
	swap_chain_extent_ = extent;
}

void VulkanRTCore::CreateSwapChainImageViews()
{
	swap_chain_image_views_.resize(swap_chain_images_.size());

	for (size_t i = 0; i < swap_chain_images_.size(); i++)
	{
		VkImageViewCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = swap_chain_images_[i];
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = swap_chain_image_format_;
		create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = 1;

		ASSERT_VK_RESULT(vkCreateImageView(device_, &create_info, nullptr, &swap_chain_image_views_[i]));
	}
}

void VulkanRTCore::CreateDescriptorSetLayout()
{
	//TODO: When we want new descriptors we need to change layout

	VkDescriptorSetLayoutBinding acceleration_structure_layout_binding{};
	acceleration_structure_layout_binding.binding = 0;
	acceleration_structure_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	acceleration_structure_layout_binding.descriptorCount = 1;
	acceleration_structure_layout_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	VkDescriptorSetLayoutBinding output_image_layout_binding{};
	output_image_layout_binding.binding = 1;
	output_image_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	output_image_layout_binding.descriptorCount = 1;
	output_image_layout_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

	VkDescriptorSetLayoutBinding uniform_buffer_binding{};
	uniform_buffer_binding.binding = 2;
	uniform_buffer_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniform_buffer_binding.descriptorCount = 1;
	uniform_buffer_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	VkDescriptorSetLayoutBinding vertex_data_addresses_binding{};
	vertex_data_addresses_binding.binding = 3;
	vertex_data_addresses_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	vertex_data_addresses_binding.descriptorCount = 1;
	vertex_data_addresses_binding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	VkDescriptorSetLayoutBinding texture_binding{};
	texture_binding.binding = 4;
	texture_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	texture_binding.descriptorCount = 1;
	texture_binding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	std::vector<VkDescriptorSetLayoutBinding> bindings = { acceleration_structure_layout_binding, output_image_layout_binding, uniform_buffer_binding, vertex_data_addresses_binding, texture_binding };

	VkDescriptorSetLayoutCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.bindingCount = static_cast<uint32_t>(bindings.size());
	create_info.pBindings = bindings.data();

	ASSERT_VK_RESULT(vkCreateDescriptorSetLayout(device_, &create_info, nullptr, &descriptor_set_layout_));
}

void VulkanRTCore::CreateDescriptorSet()
{
	//TODO: When we want new descriptors we need to change set
	std::vector<VkDescriptorPoolSize> pool_sizes({
			{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, static_cast<uint32_t>(FRAMES_IN_FLIGHT)},
			{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, static_cast<uint32_t>(FRAMES_IN_FLIGHT)},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(FRAMES_IN_FLIGHT)},
			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<uint32_t>(FRAMES_IN_FLIGHT)},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(FRAMES_IN_FLIGHT)}
		});

	VkDescriptorPoolCreateInfo descriptor_pool_info{};
	descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool_info.maxSets = static_cast<uint32_t>(FRAMES_IN_FLIGHT);
	descriptor_pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
	descriptor_pool_info.pPoolSizes = pool_sizes.data();

	ASSERT_VK_RESULT(vkCreateDescriptorPool(device_, &descriptor_pool_info, nullptr, &descriptor_pool_));

	std::vector<VkDescriptorSetLayout> layouts(FRAMES_IN_FLIGHT, descriptor_set_layout_);
	VkDescriptorSetAllocateInfo descriptor_set_allocate_info{};
	descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_set_allocate_info.descriptorPool = descriptor_pool_;
	descriptor_set_allocate_info.descriptorSetCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT);
	descriptor_set_allocate_info.pSetLayouts = layouts.data();

	descriptor_sets_.resize(FRAMES_IN_FLIGHT);
	ASSERT_VK_RESULT(vkAllocateDescriptorSets(device_, &descriptor_set_allocate_info, descriptor_sets_.data()));
}

void VulkanRTCore::CreateRayTracingPipeline()
{
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &descriptor_set_layout_;

	ASSERT_VK_RESULT(vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr, &pipeline_layout_));

	// TODO: When we want new shaders we need to change pipeline
	// idea how to do it in future from renderer:
	// CreateShaderStage(path_to_shader, stage); - return shaderID: 0,1,2..., internal vector storing VkPipelineShaderStageCreateInfo
	// CreateShaderGroup(groupType, closestHitShaderID, generalShaderID, anyHitShaderID, intersectionShaderID); internal vector storing VkRayTracingShaderGroupCreateInfoKHR
	// after we are done just call CreateRayTracingPipeline()
	// smth similar for descriptor sets Layout and other needed stuff
	VkShaderModule rgen_shader_module = CreateShaderModule("res/shaders/ray-generation.rgen");
	VkShaderModule rmiss_shader_module = CreateShaderModule("res/shaders/ray-miss.rmiss");
	VkShaderModule rchit_shader_module = CreateShaderModule("res/shaders/ray-closest-hit.rchit");

	VkPipelineShaderStageCreateInfo rgen_shader_stage_info = {};
	rgen_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	rgen_shader_stage_info.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	rgen_shader_stage_info.module = rgen_shader_module;
	rgen_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo rmiss_shader_stage_info = {};
	rmiss_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	rmiss_shader_stage_info.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
	rmiss_shader_stage_info.module = rmiss_shader_module;
	rmiss_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo rchit_shader_stage_info = {};
	rchit_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	rchit_shader_stage_info.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
	rchit_shader_stage_info.module = rchit_shader_module;
	rchit_shader_stage_info.pName = "main";

	std::vector<VkPipelineShaderStageCreateInfo> shader_stages = { rgen_shader_stage_info, rmiss_shader_stage_info, rchit_shader_stage_info };

	VkRayTracingShaderGroupCreateInfoKHR ray_gen_group = {};
	ray_gen_group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	ray_gen_group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	ray_gen_group.generalShader = 0;
	ray_gen_group.closestHitShader = VK_SHADER_UNUSED_KHR;
	ray_gen_group.anyHitShader = VK_SHADER_UNUSED_KHR;
	ray_gen_group.intersectionShader = VK_SHADER_UNUSED_KHR;

	VkRayTracingShaderGroupCreateInfoKHR ray_miss_group = {};
	ray_miss_group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	ray_miss_group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	ray_miss_group.generalShader = 1;
	ray_miss_group.closestHitShader = VK_SHADER_UNUSED_KHR;
	ray_miss_group.anyHitShader = VK_SHADER_UNUSED_KHR;
	ray_miss_group.intersectionShader = VK_SHADER_UNUSED_KHR;

	VkRayTracingShaderGroupCreateInfoKHR ray_hit_group = {};
	ray_hit_group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
	ray_hit_group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
	ray_hit_group.generalShader = VK_SHADER_UNUSED_KHR;
	ray_hit_group.closestHitShader = 2;
	ray_hit_group.anyHitShader = VK_SHADER_UNUSED_KHR;
	ray_hit_group.intersectionShader = VK_SHADER_UNUSED_KHR;

	std::vector<VkRayTracingShaderGroupCreateInfoKHR> shader_groups = { ray_gen_group, ray_miss_group, ray_hit_group };

	VkRayTracingPipelineCreateInfoKHR pipeline_info = {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
	pipeline_info.stageCount = static_cast<uint32_t>(shader_stages.size());
	pipeline_info.pStages = shader_stages.data();
	pipeline_info.groupCount = static_cast<uint32_t>(shader_groups.size());
	pipeline_info.pGroups = shader_groups.data();
	pipeline_info.maxPipelineRayRecursionDepth = 8 + 1;	//TODO 1
	pipeline_info.layout = pipeline_layout_;

	sbt_group_count_ = shader_groups.size();

	ASSERT_VK_RESULT(vkCreateRayTracingPipelinesKHR(device_, nullptr, nullptr, 1, &pipeline_info, nullptr, &ray_tracing_pipeline_));

	vkDestroyShaderModule(device_, rgen_shader_module, nullptr);
	vkDestroyShaderModule(device_, rmiss_shader_module, nullptr);
	vkDestroyShaderModule(device_, rchit_shader_module, nullptr);
}

uint32_t AlignTo(uint32_t value, uint32_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

void VulkanRTCore::CreateShaderBindingTable()
{
	const uint32_t sbt_handle_size = ray_tracing_properties_.pipeline_properties.shaderGroupHandleSize;
	const uint32_t sbt_handle_alignment = ray_tracing_properties_.pipeline_properties.shaderGroupHandleAlignment;
	const uint32_t sbt_handle_size_aligned = AlignTo(sbt_handle_size, sbt_handle_alignment);
	const uint32_t sbt_size = sbt_group_count_ * sbt_handle_size_aligned;

	std::vector<uint8_t> sbt_handles(sbt_size);
	ASSERT_VK_RESULT(vkGetRayTracingShaderGroupHandlesKHR(device_, ray_tracing_pipeline_, 0, sbt_group_count_, sbt_size, sbt_handles.data()));

	// TODO: When we want new shaders, this should be done better, sbt_handles ptr should be calculated automatically
	sbt_ray_gen_ = CreateDeviceBufferWithData(sbt_handles.data(), sbt_handle_size, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR, true);
	sbt_ray_miss_ = CreateDeviceBufferWithData(sbt_handles.data() + sbt_handle_size_aligned, sbt_handle_size, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR, true);
	sbt_ray_hit_ = CreateDeviceBufferWithData(sbt_handles.data() + 2 * sbt_handle_size_aligned, sbt_handle_size, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR, true);
}

void VulkanRTCore::CreateRenderBuffer()
{
	render_buffers_.resize(FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
		render_buffers_[i] = CreateImage(swap_chain_extent_.width, swap_chain_extent_.height, swap_chain_image_format_,
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, true);
}

void VulkanRTCore::CreateTexture(const std::string& path)
{
	stbi_set_flip_vertically_on_load(1);
	int width, height, channels_in_file;
	unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels_in_file, 4);

	if (!data)
		throw std::runtime_error("failed to load texture image!");

	texture_ = CreateImageWithData(data, width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT);

	stbi_image_free(data);
}

void VulkanRTCore::CreateTextureSampler()
{
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(physical_device_, &properties);

	VkSamplerCreateInfo sampler_info{};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_NEAREST;	//TODO VK_FILTER_LINEAR  VK_FILTER_CUBIC_IMG VK_FILTER_CUBIC_EXT
	sampler_info.minFilter = VK_FILTER_NEAREST;
	//sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	//sampler_info.mipLodBias = 0.0f;
	sampler_info.anisotropyEnable = VK_TRUE;
	sampler_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	//sampler_info.compareEnable = VK_FALSE;
	//sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	//sampler_info.minLod = 0.0f;
	//sampler_info.maxLod = 0.0f;
	//sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.unnormalizedCoordinates = VK_FALSE;

	ASSERT_VK_RESULT(vkCreateSampler(device_, &sampler_info, nullptr, &texture_sampler_));
}

void VulkanRTCore::CreateCommandBuffer()
{
	VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
	command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_allocate_info.commandPool = command_pool_;
	command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_allocate_info.commandBufferCount = FRAMES_IN_FLIGHT;

	command_buffers_.resize(FRAMES_IN_FLIGHT);
	ASSERT_VK_RESULT(vkAllocateCommandBuffers(device_, &command_buffer_allocate_info, command_buffers_.data()));
}

void VulkanRTCore::CreateUniformBuffer()
{
	uniform_buffers_.resize(FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
		uniform_buffers_[i] = CreateDeviceBufferWithHostAccess(sizeof(UniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, false);
}

void VulkanRTCore::CreateSyncObjects()
{
	VkSemaphoreCreateInfo semaphore_info{};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_info{};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	image_available_semaphores_.resize(FRAMES_IN_FLIGHT);
	render_finished_semaphores_.resize(FRAMES_IN_FLIGHT);
	in_flight_fences_.resize(FRAMES_IN_FLIGHT);
	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		ASSERT_VK_RESULT(vkCreateSemaphore(device_, &semaphore_info, nullptr, &image_available_semaphores_[i]));
		ASSERT_VK_RESULT(vkCreateSemaphore(device_, &semaphore_info, nullptr, &render_finished_semaphores_[i]));
		ASSERT_VK_RESULT(vkCreateFence(device_, &fence_info, nullptr, &in_flight_fences_[i]));
	}
}

void VulkanRTCore::Cleanup()
{
	ASSERT_VK_RESULT(vkDeviceWaitIdle(device_));

	ProcessPendingCleanups();

	CleanupSwapChain();

	vertex_buffer_addresses_.Free(allocator_);

	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(device_, image_available_semaphores_[i], nullptr);
		vkDestroySemaphore(device_, render_finished_semaphores_[i], nullptr);
		vkDestroyFence(device_, in_flight_fences_[i], nullptr);

		uniform_buffers_[i].Free(allocator_);
	}

	tlas_.Free(allocator_);
	vkDestroyAccelerationStructureKHR(device_, tlas_.as, nullptr);

	texture_.Free(allocator_, device_, nullptr);
	vkDestroySampler(device_, texture_sampler_, nullptr);

	sbt_ray_gen_.Free(allocator_);
	sbt_ray_miss_.Free(allocator_);
	sbt_ray_hit_.Free(allocator_);

	vkDestroyPipeline(device_, ray_tracing_pipeline_, nullptr);
	vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);

	vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);
	vkDestroyDescriptorSetLayout(device_, descriptor_set_layout_, nullptr);

	vkDestroyCommandPool(device_, command_pool_, nullptr);

	vmaDestroyAllocator(allocator_);

	vkDestroyDevice(device_, nullptr);

	vkDestroySurfaceKHR(instance_, surface_, nullptr);

	if (enable_validation_layers_)
		DestroyDebugUtilsMessengerEXT(instance_, debug_messenger_, nullptr);

	vkDestroyInstance(instance_, nullptr);
}

BottomLevelAccelerationStructure VulkanRTCore::BuildBLAS(const std::vector<float>& vertices, const std::vector<unsigned int>& indices)
{
	BottomLevelAccelerationStructure acceleration_structure;

	queue_mutex_.lock();
	acceleration_structure.vertex_data = CreateDeviceBufferWithData(vertices.data(), sizeof(float) * (uint32_t)vertices.size(),
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true);

	Buffer index_buffer = CreateDeviceBufferWithData(indices.data(), sizeof(uint32_t) * (uint32_t)indices.size(),
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, true);
	queue_mutex_.unlock();

	VkDeviceOrHostAddressConstKHR vertex_buffer_device_address = {};
	vertex_buffer_device_address.deviceAddress = GetBufferDeviceAddress(acceleration_structure.vertex_data.buffer);
	acceleration_structure.vertex_handle = vertex_buffer_device_address.deviceAddress;

	VkDeviceOrHostAddressConstKHR index_buffer_device_address = {};
	index_buffer_device_address.deviceAddress = GetBufferDeviceAddress(index_buffer.buffer);

	VkAccelerationStructureGeometryKHR as_geometry_info = {};
	as_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	as_geometry_info.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	as_geometry_info.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	as_geometry_info.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	as_geometry_info.geometry.triangles.vertexData = vertex_buffer_device_address;
	as_geometry_info.geometry.triangles.vertexStride = sizeof(float) * 5;	//TODO U KNOW WHAT
	as_geometry_info.geometry.triangles.maxVertex = vertices.size();
	as_geometry_info.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	as_geometry_info.geometry.triangles.indexData = index_buffer_device_address;
	as_geometry_info.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

	VkAccelerationStructureBuildGeometryInfoKHR as_build_geometry_info = {};
	as_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	as_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	as_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	as_build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	as_build_geometry_info.geometryCount = 1;
	as_build_geometry_info.pGeometries = &as_geometry_info;

	const uint32_t primitive_count = indices.size() / 3;
	VkAccelerationStructureBuildSizesInfoKHR as_build_sizes_info = {};
	as_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	vkGetAccelerationStructureBuildSizesKHR(device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &as_build_geometry_info, &primitive_count, &as_build_sizes_info);

	acceleration_structure.buffer = CreateDeviceBuffer(as_build_sizes_info.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, false);

	VkAccelerationStructureCreateInfoKHR as_info = {};
	as_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	as_info.buffer = acceleration_structure.VkBuffer();
	as_info.size = as_build_sizes_info.accelerationStructureSize;
	as_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

	ASSERT_VK_RESULT(vkCreateAccelerationStructureKHR(device_, &as_info, nullptr, &acceleration_structure.as));

	Buffer scratch_buffer = CreateDeviceBuffer(as_build_sizes_info.buildScratchSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, true);

	as_build_geometry_info.dstAccelerationStructure = acceleration_structure.as;
	as_build_geometry_info.scratchData.deviceAddress = GetBufferDeviceAddress(scratch_buffer.buffer);

	VkAccelerationStructureBuildRangeInfoKHR as_build_range_info = {};
	as_build_range_info.primitiveCount = primitive_count;
	as_build_range_info.primitiveOffset = 0;
	as_build_range_info.firstVertex = 0;
	as_build_range_info.transformOffset = 0;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> as_build_range_infos = { &as_build_range_info };

	VkFenceCreateInfo fence_info{};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	VkFence fence;
	ASSERT_VK_RESULT(vkCreateFence(device_, &fence_info, nullptr, &fence));
	StupidFence stupid_fence(fence, 2);

	acceleration_structure.build_status = stupid_fence;

	DeferredCleanup cleanup;
	cleanup.buffers_to_free.push_back(index_buffer);
	cleanup.buffers_to_free.push_back(scratch_buffer);
	cleanup.ready_for_cleanup = stupid_fence;

	queue_mutex_.lock();
	cleanup.command_buffer = BeginSingleTimeCommands();

	vkCmdBuildAccelerationStructuresKHR(cleanup.command_buffer, 1, &as_build_geometry_info, as_build_range_infos.data());

	EndSingleTimeCommands(cleanup.command_buffer, acceleration_structure.build_status);
	queue_mutex_.unlock();

	cleanup_mutex_.lock();
	pending_cleanups_.push_back(cleanup);
	cleanup_mutex_.unlock();

	//vertex_buffer.Free(allocator_);
	//index_buffer.Free(allocator_);
	//scratch_buffer.Free(allocator_);

	VkAccelerationStructureDeviceAddressInfoKHR as_device_address_info = {};
	as_device_address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	as_device_address_info.accelerationStructure = acceleration_structure.as;
	acceleration_structure.handle = vkGetAccelerationStructureDeviceAddressKHR(device_, &as_device_address_info);

	if (acceleration_structure.handle == 0)
		std::cout << "Invalid Handle to BLAS" << std::endl;

	return acceleration_structure;
}

bool VulkanRTCore::IsBLASBuilded(BottomLevelAccelerationStructure acceleration_structure)
{
	return vkGetFenceStatus(device_, acceleration_structure.build_status) == VK_SUCCESS;
}

int VulkanRTCore::ProcessPendingCleanups()	// async BLAS build
{
	size_t swapable = 0;
	std::lock_guard<std::mutex> lock(cleanup_mutex_);

	while (swapable < pending_cleanups_.size())
	{
		VkResult result = vkGetFenceStatus(device_, pending_cleanups_.back().ready_for_cleanup);

		if (result == VK_SUCCESS) 
		{
			// GPU work is done, free the buffers associated with this cleanup
			for (auto& buffer : pending_cleanups_.back().buffers_to_free) 
				buffer.Free(allocator_);
			queue_mutex_.lock();
			vkFreeCommandBuffers(device_, command_pool_, 1, &pending_cleanups_.back().command_buffer);
			pending_cleanups_.back().ready_for_cleanup.Free(device_, nullptr);
			queue_mutex_.unlock();

			// Pop the last element, which has now been processed
			pending_cleanups_.pop_back();
		}
		else if (result == VK_NOT_READY) 
		{
			// Swap the current(last) element with the first++, to save cleanup for next function call
			std::swap(pending_cleanups_.back(), pending_cleanups_[swapable++]);
		}
	}
	return pending_cleanups_.size();
}

void VulkanRTCore::FreeBLAS(BottomLevelAccelerationStructure acceleration_structure)
{
	for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
		ASSERT_VK_RESULT(vkWaitForFences(device_, 1, &in_flight_fences_[i], VK_TRUE, UINT64_MAX));
	acceleration_structure.Free(allocator_, device_, nullptr);
	vkDestroyAccelerationStructureKHR(device_, acceleration_structure.as, nullptr);
}

void VulkanRTCore::BuildTLAS(std::vector<BottomLevelAccelerationStructure> blases)
{
	//while (ProcessPendingCleanups())
	//	std::cout << "Waited" << std::endl;	//TODO better wait for all blases being build

	VkTransformMatrixKHR instance_transform = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f
	};

	std::vector<VkAccelerationStructureInstanceKHR> instances;
	std::vector<VkDeviceAddress> vertex_buffer_addresses;
	instances.reserve(blases.size());
	vertex_buffer_addresses.reserve(blases.size());

	VkAccelerationStructureInstanceKHR instance = {};
	instance.transform = instance_transform;
	instance.mask = 0xFF;
	instance.instanceShaderBindingTableRecordOffset = 0;
	instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;	//TODO CULL ON

	for (size_t i = 0; i < blases.size(); ++i)
	{
		if (!blases[i].builded)
			vkWaitForFences(device_, 1, &blases[i].build_status.fence, VK_TRUE, UINT64_MAX);
		instance.instanceCustomIndex = i;
		instance.accelerationStructureReference = blases[i].handle;
		instances.push_back(instance);
		vertex_buffer_addresses.push_back(blases[i].vertex_handle);
	}

	Buffer instance_buffer = CreateDeviceBufferWithData(instances.data(), sizeof(VkAccelerationStructureInstanceKHR) * instances.size(),
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, true);

	VkDeviceOrHostAddressConstKHR instance_device_address = {};
	instance_device_address.deviceAddress = GetBufferDeviceAddress(instance_buffer.buffer);

	if (vertex_buffer_addresses_.buffer != VK_NULL_HANDLE)
		vertex_buffer_addresses_.Free(allocator_);

	vertex_buffer_addresses_ = CreateDeviceBufferWithData(vertex_buffer_addresses.data(), sizeof(VkDeviceAddress) * vertex_buffer_addresses.size(),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, false);

	VkAccelerationStructureGeometryKHR as_geometry_info = {};
	as_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	as_geometry_info.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	as_geometry_info.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	as_geometry_info.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	as_geometry_info.geometry.instances.arrayOfPointers = VK_FALSE;
	as_geometry_info.geometry.instances.data = instance_device_address;

	VkAccelerationStructureBuildGeometryInfoKHR as_build_geometry_info = {};
	as_build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	as_build_geometry_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	as_build_geometry_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	as_build_geometry_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	as_build_geometry_info.geometryCount = 1;
	as_build_geometry_info.pGeometries = &as_geometry_info;

	const uint32_t instance_count = static_cast<uint32_t>(instances.size());
	VkAccelerationStructureBuildSizesInfoKHR as_build_sizes_info = {};
	as_build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	vkGetAccelerationStructureBuildSizesKHR(device_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &as_build_geometry_info, &instance_count, &as_build_sizes_info);

	if (tlas_.as != VK_NULL_HANDLE)
	{
		tlas_.Free(allocator_);
		vkDestroyAccelerationStructureKHR(device_, tlas_.as, nullptr);
	}

	tlas_.buffer = CreateDeviceBuffer(as_build_sizes_info.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, false);

	VkAccelerationStructureCreateInfoKHR as_info = {};
	as_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	as_info.buffer = tlas_.VkBuffer();
	as_info.size = as_build_sizes_info.accelerationStructureSize;
	as_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

	ASSERT_VK_RESULT(vkCreateAccelerationStructureKHR(device_, &as_info, nullptr, &tlas_.as));

	Buffer scratch_buffer = CreateDeviceBuffer(as_build_sizes_info.buildScratchSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, true);

	as_build_geometry_info.dstAccelerationStructure = tlas_.as;
	as_build_geometry_info.scratchData.deviceAddress = GetBufferDeviceAddress(scratch_buffer.buffer);	// TODO VMA go with alignment of 16, while 128 is required, this is simple fix
																													// whether this is fault of my code, or i doesn't set smth, or actualy not my fault...
																													// I don't have time for it, this simple and fast fix works for now, if something won't work look at this
																													// btw BLAS building doesn't "fire" this error
	VkAccelerationStructureBuildRangeInfoKHR as_build_range_info = {};
	as_build_range_info.primitiveCount = instance_count;
	as_build_range_info.primitiveOffset = 0;
	as_build_range_info.firstVertex = 0;
	as_build_range_info.transformOffset = 0;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> as_build_range_infos = { &as_build_range_info };

	VkCommandBuffer command_buffer = BeginSingleTimeCommands();

	vkCmdBuildAccelerationStructuresKHR(command_buffer, 1, &as_build_geometry_info, as_build_range_infos.data());

	EndSingleTimeCommands(command_buffer);

	instance_buffer.Free(allocator_);
	scratch_buffer.Free(allocator_);
}

void VulkanRTCore::UpdateDescriptorSet()
{
	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		VkWriteDescriptorSetAccelerationStructureKHR descriptor_as{};
		descriptor_as.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
		descriptor_as.accelerationStructureCount = 1;
		descriptor_as.pAccelerationStructures = &tlas_.as;

		VkWriteDescriptorSet write_as{};
		write_as.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_as.pNext = &descriptor_as;
		write_as.dstSet = descriptor_sets_[i];
		write_as.dstBinding = 0;
		write_as.descriptorCount = 1;
		write_as.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

		VkDescriptorImageInfo descriptor_output_image{};
		descriptor_output_image.sampler = VK_NULL_HANDLE;
		descriptor_output_image.imageView = render_buffers_[i].image_view;
		descriptor_output_image.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet write_render_buffer{};
		write_render_buffer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_render_buffer.dstSet = descriptor_sets_[i];
		write_render_buffer.dstBinding = 1;
		write_render_buffer.descriptorCount = 1;
		write_render_buffer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		write_render_buffer.pImageInfo = &descriptor_output_image;

		VkDescriptorBufferInfo descriptor_uniform_buffer{};
		descriptor_uniform_buffer.buffer = uniform_buffers_[i].buffer;
		descriptor_uniform_buffer.offset = 0;
		descriptor_uniform_buffer.range = sizeof(UniformBuffer);

		VkWriteDescriptorSet write_uniform_buffer{};
		write_uniform_buffer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_uniform_buffer.dstSet = descriptor_sets_[i];
		write_uniform_buffer.dstBinding = 2;
		write_uniform_buffer.descriptorCount = 1;
		write_uniform_buffer.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write_uniform_buffer.pBufferInfo = &descriptor_uniform_buffer;

		VkDescriptorBufferInfo descriptor_storage_buffer{};
		descriptor_storage_buffer.buffer = vertex_buffer_addresses_.buffer;
		descriptor_storage_buffer.offset = 0;
		descriptor_storage_buffer.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet write_vertex_data_addresses_buffer{};
		write_vertex_data_addresses_buffer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_vertex_data_addresses_buffer.dstSet = descriptor_sets_[i];
		write_vertex_data_addresses_buffer.dstBinding = 3;
		write_vertex_data_addresses_buffer.descriptorCount = 1;
		write_vertex_data_addresses_buffer.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		write_vertex_data_addresses_buffer.pBufferInfo = &descriptor_storage_buffer;

		VkDescriptorImageInfo descriptor_texture_image{};
		descriptor_texture_image.sampler = texture_sampler_;
		descriptor_texture_image.imageView = texture_.image_view;
		descriptor_texture_image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet write_texture_buffer{};
		write_texture_buffer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_texture_buffer.dstSet = descriptor_sets_[i];
		write_texture_buffer.dstBinding = 4;
		write_texture_buffer.descriptorCount = 1;
		write_texture_buffer.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write_texture_buffer.pImageInfo = &descriptor_texture_image;

		std::vector<VkWriteDescriptorSet> write_descriptors = { write_as, write_render_buffer, write_uniform_buffer, write_vertex_data_addresses_buffer, write_texture_buffer };

		vkUpdateDescriptorSets(device_, static_cast<uint32_t>(write_descriptors.size()), write_descriptors.data(), 0, nullptr);
	}
}

void VulkanRTCore::RecordCommandBuffer(uint32_t swap_chain_image_index)
{
	VkCommandBufferBeginInfo command_buffer_begin_info{};
	command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	ASSERT_VK_RESULT(vkBeginCommandBuffer(command_buffers_[frame_in_flight], &command_buffer_begin_info));

	// transition offscreen buffer into shader writeable state
	TransitionImageLayout(command_buffers_[frame_in_flight], render_buffers_[frame_in_flight].image, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	vkCmdBindPipeline(command_buffers_[frame_in_flight], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, ray_tracing_pipeline_);
	vkCmdBindDescriptorSets(command_buffers_[frame_in_flight], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline_layout_, 0, 1, &descriptor_sets_[frame_in_flight], 0, 0);

	const uint32_t sbt_handle_size = ray_tracing_properties_.pipeline_properties.shaderGroupHandleSize;
	const uint32_t sbt_handle_alignment = ray_tracing_properties_.pipeline_properties.shaderGroupHandleAlignment;
	const uint32_t sbt_handle_size_aligned = AlignTo(sbt_handle_size, sbt_handle_alignment);

	VkStridedDeviceAddressRegionKHR ray_gen = {};
	ray_gen.deviceAddress = GetBufferDeviceAddress(sbt_ray_gen_.buffer);
	ray_gen.stride = sbt_handle_size_aligned;
	ray_gen.size = sbt_handle_size;

	VkStridedDeviceAddressRegionKHR ray_miss = {};
	ray_miss.deviceAddress = GetBufferDeviceAddress(sbt_ray_miss_.buffer);
	ray_miss.stride = sbt_handle_size_aligned;
	ray_miss.size = sbt_handle_size;

	VkStridedDeviceAddressRegionKHR ray_hit = {};
	ray_hit.deviceAddress = GetBufferDeviceAddress(sbt_ray_hit_.buffer);
	ray_hit.stride = sbt_handle_size_aligned;
	ray_hit.size = sbt_handle_size;

	VkStridedDeviceAddressRegionKHR ray_call = {};

	vkCmdTraceRaysKHR(command_buffers_[frame_in_flight], &ray_gen, &ray_miss, &ray_hit, &ray_call, swap_chain_extent_.width, swap_chain_extent_.height, 1);

	// transition swapchain image into copy destination state
	TransitionImageLayout(command_buffers_[frame_in_flight], swap_chain_images_[swap_chain_image_index], 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// transition offscreen buffer into copy source state
	TransitionImageLayout(command_buffers_[frame_in_flight], render_buffers_[frame_in_flight].image, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
		VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	VkImageCopy copy_region = {};
	copy_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy_region.srcSubresource.mipLevel = 0;
	copy_region.srcSubresource.baseArrayLayer = 0;
	copy_region.srcSubresource.layerCount = 1;
	copy_region.srcOffset = { 0, 0, 0 };
	copy_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copy_region.dstSubresource.mipLevel = 0;
	copy_region.dstSubresource.baseArrayLayer = 0;
	copy_region.dstSubresource.layerCount = 1;
	copy_region.dstOffset = { 0, 0, 0 };
	copy_region.extent.depth = 1;
	copy_region.extent.width = swap_chain_extent_.width;
	copy_region.extent.height = swap_chain_extent_.height;

	// copy offscreen buffer into swapchain image
	vkCmdCopyImage(command_buffers_[frame_in_flight], render_buffers_[frame_in_flight].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		swap_chain_images_[swap_chain_image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

	// transition swapchain image into presentable state
	TransitionImageLayout(command_buffers_[frame_in_flight], swap_chain_images_[swap_chain_image_index], VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	ASSERT_VK_RESULT(vkEndCommandBuffer(command_buffers_[frame_in_flight]));
}

void VulkanRTCore::Render(UniformBuffer uniform_buffer)
{
	ASSERT_VK_RESULT(vkWaitForFences(device_, 1, &in_flight_fences_[frame_in_flight], VK_TRUE, UINT64_MAX));

	uint32_t image_index;
	VkResult result = vkAcquireNextImageKHR(device_, swap_chain_, UINT64_MAX, image_available_semaphores_[frame_in_flight], VK_NULL_HANDLE, &image_index);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		window_resized_ = false;
		RecreateSwapChain();
		return;
	}
	else if ((result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR))
		ASSERT_VK_RESULT(result);

	memcpy(uniform_buffers_[frame_in_flight].buffer_ptr, &uniform_buffer, sizeof(UniformBuffer));
	ASSERT_VK_RESULT(vkResetFences(device_, 1, &in_flight_fences_[frame_in_flight]));
	ASSERT_VK_RESULT(vkResetCommandBuffer(command_buffers_[frame_in_flight], 0));
	RecordCommandBuffer(image_index);

	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };//TODO i dont know which one to use actually

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &image_available_semaphores_[frame_in_flight];
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffers_[frame_in_flight];
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &render_finished_semaphores_[frame_in_flight];

	ASSERT_VK_RESULT(vkQueueSubmit(queue_, 1, &submit_info, in_flight_fences_[frame_in_flight]));

	VkPresentInfoKHR present_info{};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &render_finished_semaphores_[frame_in_flight];
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swap_chain_;
	present_info.pImageIndices = &image_index;

	result = vkQueuePresentKHR(queue_, &present_info);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window_resized_)
	{
		window_resized_ = false;
		RecreateSwapChain();
	}
	else
		ASSERT_VK_RESULT(result);

	frame_in_flight = (frame_in_flight + 1) % FRAMES_IN_FLIGHT;
}

void VulkanRTCore::CleanupSwapChain()
{
	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
		render_buffers_[i].Free(allocator_, device_, nullptr);

	for (auto image_view : swap_chain_image_views_)
		vkDestroyImageView(device_, image_view, nullptr);
	vkDestroySwapchainKHR(device_, swap_chain_, nullptr);
}

void VulkanRTCore::RecreateSwapChain()
{
	if (!window_->NotMinimized())
		return;

	for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
		ASSERT_VK_RESULT(vkWaitForFences(device_, 1, &in_flight_fences_[i], VK_TRUE, UINT64_MAX));
	CleanupSwapChain();

	CreateSwapChain();
	CreateSwapChainImageViews();
	CreateRenderBuffer();
	UpdateDescriptorSet();
}

std::vector<const char*> VulkanRTCore::GetRequiredExtensions()
{
	uint32_t glfw_extension_count = 0;
	const char** glfw_extensions = window_->GetRequiredInstanceExtensions(&glfw_extension_count);

	std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

	if (enable_validation_layers_)
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return extensions;
}

bool VulkanRTCore::CheckValidationLayerSupport()
{
	uint32_t layer_count;
	ASSERT_VK_RESULT(vkEnumerateInstanceLayerProperties(&layer_count, nullptr));

	std::vector<VkLayerProperties> available_layers(layer_count);
	ASSERT_VK_RESULT(vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data()));

	for (const char* layer_name : validation_layers_)
	{
		bool layer_found = false;

		for (const auto& layer_properties : available_layers)
		{
			if (strcmp(layer_name, layer_properties.layerName) == 0)
			{
				layer_found = true;
				break;
			}
		}

		if (!layer_found)
			return false;
	}

	return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

void VulkanRTCore::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info)
{
	// Fill in debug messenger create info
	create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_info.pfnUserCallback = DebugCallback;
}

bool VulkanRTCore::IsDeviceSuitable(VkPhysicalDevice device)
{
	// Check if the device have queue with required capabilities
	bool queue_support = FindQueueFamilies(device).IsComplete();

	// Check if the device supports required extensions
	bool extensions_supported = CheckDeviceExtensionSupport(device);

	// Check for ray tracing support
	bool ray_tracing_supported = false;
	if (extensions_supported)
	{
		VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_structure_features{};
		acceleration_structure_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;

		VkPhysicalDeviceRayTracingPipelineFeaturesKHR ray_tracing_pipeline_features{};
		ray_tracing_pipeline_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
		ray_tracing_pipeline_features.pNext = &acceleration_structure_features;

		VkPhysicalDeviceFeatures2 device_features_2{};
		device_features_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		device_features_2.pNext = &ray_tracing_pipeline_features;

		vkGetPhysicalDeviceFeatures2(device, &device_features_2);

		ray_tracing_supported = ray_tracing_pipeline_features.rayTracingPipeline && acceleration_structure_features.accelerationStructure;
	}

	return queue_support && extensions_supported && ray_tracing_supported;
}

bool VulkanRTCore::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extension_count;
	ASSERT_VK_RESULT(vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr));

	std::vector<VkExtensionProperties> available_extensions(extension_count);
	ASSERT_VK_RESULT(vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data()));

	for (const char* extension_name : device_extensions_)
	{
		bool extension_found = false;

		for (const auto& extension : available_extensions)
		{
			if (strcmp(extension_name, extension.extensionName) == 0)
			{
				extension_found = true;
				break;
			}
		}

		if (!extension_found)
			return false;
	}

	return true;
}

QueueFamilyIndices VulkanRTCore::FindQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

	int i = 0;
	for (const auto& queue_family : queue_families)
	{
		if (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT)
			indices.compute_family = i;

		VkBool32 present_support = false;
		ASSERT_VK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &present_support));

		if (indices.IsComplete() && present_support)
			break;
		i++;
	}

	return indices;
}

SwapChainSupportDetails VulkanRTCore::QuerySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;

	ASSERT_VK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities));

	uint32_t format_count;
	ASSERT_VK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, nullptr));

	if (format_count != 0)
	{
		details.formats.resize(format_count);
		ASSERT_VK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, details.formats.data()));
	}

	uint32_t present_mode_count;
	ASSERT_VK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, nullptr));

	if (present_mode_count != 0)
	{
		details.present_modes.resize(present_mode_count);
		ASSERT_VK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, details.present_modes.data()));
	}

	return details;
}

VkSurfaceFormatKHR VulkanRTCore::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats)
{
	for (const auto& available_format : available_formats)
		if (available_format.format == VK_FORMAT_B8G8R8A8_UNORM && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return available_format;

	return available_formats[0];  // If the preferred format is not available, return the first format
}

VkPresentModeKHR VulkanRTCore::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes)
{
	for (const auto& available_present_mode : available_present_modes)
		if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return available_present_mode;

	return VK_PRESENT_MODE_FIFO_KHR;  // Fallback to FIFO mode which is always available
}

VkExtent2D VulkanRTCore::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		return capabilities.currentExtent;
	else
	{
		VkExtent2D actual_extent = { static_cast<uint32_t>(window_->GetWidth()), static_cast<uint32_t>(window_->GetHeigth()) };

		actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actual_extent;
	}
}

std::vector<char> LoadShaderSource(const std::string& file_path)
{
	std::cout << "Reading: " << file_path << std::endl;

	std::ifstream file(file_path, std::ios::ate | std::ios::binary);
	if (!file.is_open())
		throw std::runtime_error("Could not open file");

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

void CompileShaderToSPV(const std::string& source_path, const std::string& output_path)
{
	std::string command = "glslangValidator  --target-env vulkan1.2 -V " + source_path + " -o " + output_path;
	int result = std::system(command.c_str());

	if (result != 0)
		throw std::runtime_error("Shader compilation failed for: " + source_path);
}

VkShaderModule VulkanRTCore::CreateShaderModule(const std::string& file_path)
{
	std::string spv_file_path = file_path + ".spv";

	bool needs_compilation = !std::filesystem::exists(spv_file_path) ||
		std::filesystem::last_write_time(file_path) > std::filesystem::last_write_time(spv_file_path);

	if (needs_compilation)
	{
		std::cout << "Compiling shader: ";
		CompileShaderToSPV(file_path, spv_file_path);
	}

	const std::vector<char> code = LoadShaderSource(spv_file_path);

	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = code.size();
	create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shader_module;
	ASSERT_VK_RESULT(vkCreateShaderModule(device_, &create_info, nullptr, &shader_module));

	return shader_module;
}

Buffer VulkanRTCore::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage, bool host_access, bool device_address)
{
	Buffer buffer;

	VkBufferCreateInfo buffer_info{};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = size;
	buffer_info.usage = usage | (device_address ? VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT : 0);
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	//ASSERT_VK_RESULT(vkCreateBuffer(device_, &buffer_info, nullptr, &buffer.buffer));

	//VkMemoryRequirements memory_requirements;
	//vkGetBufferMemoryRequirements(device_, buffer.buffer, &memory_requirements);

	//VkMemoryAllocateFlagsInfo memory_allocate_flags_info{};
	//memory_allocate_flags_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
	//memory_allocate_flags_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

	//VkMemoryAllocateInfo memory_allocate_info{};
	//memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	//memory_allocate_info.pNext = device_address ? &memory_allocate_flags_info : nullptr;
	//memory_allocate_info.allocationSize = memory_requirements.size;
	//memory_allocate_info.memoryTypeIndex = FindMemoryType(memory_requirements.memoryTypeBits, properties);

	//ASSERT_VK_RESULT(vkAllocateMemory(device_, &memory_allocate_info, nullptr, &buffer.memory));
	//ASSERT_VK_RESULT(vkBindBufferMemory(device_, buffer.buffer, buffer.memory, 0));

	VmaAllocationCreateInfo allocation_info{};
	allocation_info.usage = memory_usage;
	allocation_info.flags = host_access ? VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT : 0;
	ASSERT_VK_RESULT(vmaCreateBuffer(allocator_, &buffer_info, &allocation_info, &buffer.buffer, &buffer.allocation, nullptr));

	return buffer;
}

Buffer VulkanRTCore::CreateDeviceBuffer(VkDeviceSize size, VkBufferUsageFlags usage, bool device_address)
{
	//return CreateBuffer(size, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, device_address);
	return CreateBuffer(size, usage, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, false, device_address);
}

Buffer VulkanRTCore::CreateDeviceBufferWithData(const void* src_data, VkDeviceSize size, VkBufferUsageFlags usage, bool device_address)
{
	//Buffer staging_buffer = CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, false);
	Buffer staging_buffer = CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, true, false);

	void* dst_data = nullptr;
	//ASSERT_VK_RESULT(vkMapMemory(device_, staging_buffer.memory, 0, size, 0, &dst_data));
	ASSERT_VK_RESULT(vmaMapMemory(allocator_, staging_buffer.allocation, &dst_data));
	memcpy(dst_data, src_data, static_cast<size_t>(size));
	//vkUnmapMemory(device_, staging_buffer.memory);
	vmaUnmapMemory(allocator_, staging_buffer.allocation);

	//Buffer buffer = CreateBuffer(size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, device_address);
	Buffer buffer = CreateBuffer(size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, false, device_address);

	VkCommandBuffer command_buffer = BeginSingleTimeCommands();

	VkBufferCopy copy_region{};
	copy_region.size = size;
	vkCmdCopyBuffer(command_buffer, staging_buffer.buffer, buffer.buffer, 1, &copy_region);

	EndSingleTimeCommands(command_buffer);

	staging_buffer.Free(allocator_);

	return buffer;
}

MappedBuffer VulkanRTCore::CreateDeviceBufferWithHostAccess(VkDeviceSize size, VkBufferUsageFlags usage, bool device_address)
{
	//Buffer buffer = CreateBuffer(size, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, device_address);

	//MappedBuffer mapped_buffer;
	//mapped_buffer.buffer = buffer.buffer;
	//mapped_buffer.memory = buffer.memory;
	//ASSERT_VK_RESULT(vkMapMemory(device_, mapped_buffer.memory, 0, size, 0, &mapped_buffer.buffer_ptr));

	Buffer memory_buffer = CreateBuffer(size, usage, VMA_MEMORY_USAGE_AUTO, true, device_address);

	MappedBuffer mapped_buffer;
	mapped_buffer.buffer = memory_buffer.buffer;
	mapped_buffer.allocation = memory_buffer.allocation;
	ASSERT_VK_RESULT(vmaMapMemory(allocator_, mapped_buffer.allocation, &mapped_buffer.buffer_ptr));

	return mapped_buffer;
}

uint64_t VulkanRTCore::GetBufferDeviceAddress(VkBuffer buffer)
{
	VkBufferDeviceAddressInfoKHR buffer_device_address_info = {};
	buffer_device_address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	buffer_device_address_info.buffer = buffer;

	return vkGetBufferDeviceAddress(device_, &buffer_device_address_info);
}

ImageResource VulkanRTCore::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, bool dedicated_allocation)
{
	ImageResource image;

	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.format = format;
	image_info.extent = { width, height, 1 };
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.usage = usage;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	//ASSERT_VK_RESULT(vkCreateImage(device_, &image_info, nullptr, &image.image));

	//VkMemoryRequirements memory_requirements;
	//vkGetImageMemoryRequirements(device_, image.image, &memory_requirements);

	//VkMemoryAllocateInfo memory_allocate_info = {};
	//memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	//memory_allocate_info.allocationSize = memory_requirements.size;
	//memory_allocate_info.memoryTypeIndex = FindMemoryType(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	//ASSERT_VK_RESULT(vkAllocateMemory(device_, &memory_allocate_info, nullptr, &image.memory));
	//ASSERT_VK_RESULT(vkBindImageMemory(device_, image.image, image.memory, 0));

	VmaAllocationCreateInfo allocation_info = {};
	allocation_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	allocation_info.flags = dedicated_allocation ? VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT : 0;

	vmaCreateImage(allocator_, &image_info, &allocation_info, &image.image, &image.allocation, nullptr);

	VkImageViewCreateInfo image_view_info = {};
	image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_info.image = image.image;
	image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	image_view_info.format = format;
	image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_view_info.subresourceRange.baseMipLevel = 0;
	image_view_info.subresourceRange.levelCount = 1;
	image_view_info.subresourceRange.baseArrayLayer = 0;
	image_view_info.subresourceRange.layerCount = 1;

	ASSERT_VK_RESULT(vkCreateImageView(device_, &image_view_info, nullptr, &image.image_view));

	return image;
}

ImageResource VulkanRTCore::CreateImageWithData(const void* src_data, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage)
{
	VkDeviceSize image_size;
	if (format == VK_FORMAT_R8G8B8A8_SRGB)
		image_size = width * height * 4;
	else
	{
		image_size = width * height * 4;
		throw std::runtime_error("Unknown format for image");
	}

	//Buffer staging_buffer = CreateBuffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, false);
	Buffer staging_buffer = CreateBuffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST, true, false);

	void* dst_data = nullptr;
	//ASSERT_VK_RESULT(vkMapMemory(device_, staging_buffer.memory, 0, image_size, 0, &dst_data));
	ASSERT_VK_RESULT(vmaMapMemory(allocator_, staging_buffer.allocation, &dst_data));
	memcpy(dst_data, src_data, static_cast<size_t>(image_size));
	//vkUnmapMemory(device_, staging_buffer.memory);
	vmaUnmapMemory(allocator_, staging_buffer.allocation);

	ImageResource image = CreateImage(width, height, format, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false);

	VkCommandBuffer command_buffer = BeginSingleTimeCommands();
	TransitionImageLayout(command_buffer, image.image, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };
	vkCmdCopyBufferToImage(command_buffer, staging_buffer.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	TransitionImageLayout(command_buffer, image.image, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	EndSingleTimeCommands(command_buffer);

	staging_buffer.Free(allocator_);

	return image;
}

//uint32_t VulkanRTCore::FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties)
//{
//	VkPhysicalDeviceMemoryProperties mem_properties;
//	vkGetPhysicalDeviceMemoryProperties(physical_device_, &mem_properties);
//
//	for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
//		if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
//			return i;
//
//	throw std::runtime_error("Failed to find suitable memory type!");
//}

VkCommandBuffer VulkanRTCore::BeginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = command_pool_;
	alloc_info.commandBufferCount = 1;

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(device_, &alloc_info, &command_buffer);

	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(command_buffer, &begin_info);

	return command_buffer;
}

void VulkanRTCore::EndSingleTimeCommands(VkCommandBuffer command_buffer, VkFence fence)
{
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;

	vkQueueSubmit(queue_, 1, &submit_info, fence);
	if (fence == VK_NULL_HANDLE)
	{
		vkQueueWaitIdle(queue_);	// Bettter sync method
		vkFreeCommandBuffers(device_, command_pool_, 1, &command_buffer);
	}
}

void VulkanRTCore::TransitionImageLayout(VkCommandBuffer command_buffer, VkImage image, VkAccessFlags src_access_mask, VkPipelineStageFlags src_stage,
	VkAccessFlags dst_access_mask, VkPipelineStageFlags dst_stage, VkImageLayout old_layout, VkImageLayout new_layout)
{
	VkImageMemoryBarrier image_memory_barrier = {};
	image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	image_memory_barrier.srcAccessMask = src_access_mask;
	image_memory_barrier.dstAccessMask = dst_access_mask;
	image_memory_barrier.oldLayout = old_layout;
	image_memory_barrier.newLayout = new_layout;
	image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.image = image;
	image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_memory_barrier.subresourceRange.baseMipLevel = 0;
	image_memory_barrier.subresourceRange.levelCount = 1;
	image_memory_barrier.subresourceRange.baseArrayLayer = 0;
	image_memory_barrier.subresourceRange.layerCount = 1;

	vkCmdPipelineBarrier(command_buffer, src_stage, dst_stage,
		0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);
}

#endif