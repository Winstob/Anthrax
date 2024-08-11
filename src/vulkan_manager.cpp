/* ---------------------------------------------------------------- *\
 * vulkan_manager.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-03-26
\* ---------------------------------------------------------------- */

#include "vulkan_manager.hpp"

#include <iostream>
#include <algorithm>
#include <limits>
#include <cstring>

#define xstr(s) str(s)
#define str(s) #s

namespace Anthrax
{


// Static member allocation
GLFWwindow *VulkanManager::window_;



VulkanManager::VulkanManager()
{
	window_width_ = 800;
	window_height_ = 600;
	return;
}


VulkanManager::~VulkanManager()
{
	destroy();
	return;
}


void VulkanManager::init()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window_ = glfwCreateWindow(window_width_, window_height_, xstr(WINDOW_NAME), NULL, NULL);
	if (window_ == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return;
	}
	glfwSetWindowUserPointer(window_, this);
	glfwSetFramebufferSizeCallback(window_, framebufferSizeCallback);

	createInstance();
	createSurface();
	createAnthraxDevice();
	createSwapChain();
	createImageViews();
	createBuffers();
	createRenderPass();
	//createGraphicsPipeline();
	createFramebuffers();
	createCommandPool();
	createCommandBuffer();
	createSyncObjects();

	createComputeShader();
	createComputeCommandPool();
	createComputeCommandBuffer();

	return;
}


void VulkanManager::drawFrame()
{
	// Compute pass
	vkDeviceWaitIdle(device_.logical);
	vkResetCommandBuffer(compute_command_buffer_, 0);
	compute_shader_manager_.recordCommandBuffer(compute_command_buffer_);

	VkSubmitInfo compute_submit_info{};
	compute_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	compute_submit_info.commandBufferCount = 1;
	compute_submit_info.pCommandBuffers = &compute_command_buffer_;
	compute_submit_info.signalSemaphoreCount = 0;
	//compute_submit_info.
	
	if (vkQueueSubmit(compute_queue_, 1, &compute_submit_info, VK_NULL_HANDLE) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to submit dispatch command buffer");
	}
	vkDeviceWaitIdle(device_.logical);




	// Graphics pass
	vkWaitForFences(device_.logical, 1, &in_flight_fence_, VK_TRUE, UINT64_MAX);
	vkResetFences(device_.logical, 1, &in_flight_fence_);

	uint32_t image_index;
	vkAcquireNextImageKHR(device_.logical, swap_chain_.data(), UINT64_MAX, image_available_semaphore_, VK_NULL_HANDLE, &image_index);

	vkResetCommandBuffer(command_buffer_, 0);
	//recordCommandBuffer(command_buffer_, image_index);
	render_pass_.recordCommandBuffer(command_buffer_, swap_chain_framebuffers_[image_index]);

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore wait_semaphores[] = {image_available_semaphore_};
	VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer_;
	VkSemaphore signal_semaphores[] = {render_finished_semaphore_};
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = signal_semaphores;
	if (vkQueueSubmit(graphics_queue_, 1, &submit_info, in_flight_fence_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to submit draw command buffer");
	}


	VkPresentInfoKHR present_info{};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = signal_semaphores;
	VkSwapchainKHR swap_chains[] = { swap_chain_.data() };
	present_info.swapchainCount = 1;
	present_info.pSwapchains = swap_chains;
	present_info.pImageIndices = &image_index;
	present_info.pResults = nullptr;
	VkResult result = vkQueuePresentKHR(present_queue_, &present_info);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized_)
	{
		recreateSwapChain();
		framebuffer_resized_ = false;
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to acquire swap chain image");
	}
	glfwPollEvents();
	return;
}


void VulkanManager::destroy()
{
	vkDeviceWaitIdle(device_.logical); // Wait for any asynchronous operations to finish

	// Buffers
	raymarched_ssbo_.destroy();
	world_ssbo_.destroy();

	// Compute shader stuff
	compute_shader_manager_.destroy();
	vkDestroyCommandPool(device_.logical, compute_command_pool_, nullptr);

	destroySwapChain();

	vkDestroySemaphore(device_.logical, image_available_semaphore_, nullptr);
	vkDestroySemaphore(device_.logical, render_finished_semaphore_, nullptr);
	vkDestroyFence(device_.logical, in_flight_fence_, nullptr);
	vkDestroyCommandPool(device_.logical, command_pool_, nullptr);
	/*
	delete graphics_pipeline_;
	vkDestroyRenderPass(device_.logical, render_pass_, nullptr);
	*/
	render_pass_.destroy();
	//vkDestroyDevice(device_.logical, nullptr);
	device_.destroy();
	vkDestroySurfaceKHR(instance_, surface_, nullptr);
	vkDestroyInstance(instance_, nullptr);
	glfwDestroyWindow(window_);

	glfwTerminate();

	return;
}


void VulkanManager::createInstance()
{
	if (enable_validation_layers_ && !checkValidationLayerSupport())
	{
		throw std::runtime_error("Validation layers not available");
	}

	VkApplicationInfo app_info{};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = xstr(WINDOW_NAME);
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "Anthrax";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;

	if (enable_validation_layers_)
	{
		create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers_.size());
		create_info.ppEnabledLayerNames = validation_layers_.data();
	}
	else
	{
		create_info.enabledLayerCount = 0;
	}

	auto extensions = getRequiredInstanceExtensions();
	create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	create_info.ppEnabledExtensionNames = extensions.data();

	if (vkCreateInstance(&create_info, nullptr, &instance_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vulkan instance");
	}

	return;
}


void VulkanManager::createSurface()
{
	if (glfwCreateWindowSurface(instance_, window_, nullptr, &surface_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create GLFW window surface");
		glfwTerminate();
	}
	return;
}


void VulkanManager::createAnthraxDevice()
{
	VkPhysicalDevice physical_device = pickPhysicalDevice();
	VkDevice logical_device = createLogicalDevice(physical_device);
	device_ = Device(physical_device, logical_device);

	return;
}


VkPhysicalDevice VulkanManager::pickPhysicalDevice()
{
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	std::vector<VkPhysicalDevice> suitable_devices;
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);
	if (device_count == 0)
		throw std::runtime_error("No compatible GPUs found");

	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());

	for (const auto &device : devices)
	{
		VkPhysicalDeviceProperties device_properties;
		vkGetPhysicalDeviceProperties(device, &device_properties);
		//std::cout << device_properties.deviceName << std::endl;
		if (isDeviceSuitable(device))
		{
			suitable_devices.push_back(device);
		}
	}

	if (suitable_devices.size() == 0)
	{
		throw std::runtime_error("Failed to find suitable GPU");
	}

	physical_device = suitable_devices[0];
	return physical_device;
}


bool VulkanManager::isDeviceSuitable(VkPhysicalDevice device)
{
	/*
	VkPhysicalDeviceProperties device_properties;
	VkPhysicalDeviceFeatures device_features;
	vkGetPhysicalDeviceProperties(device, &device_properties);
	vkGetPhysicalDeviceFeatures(device, &device_features);

	return device_features.geometryShader;
	*/
	QueueFamilyIndices indices = findQueueFamilies(device);
	bool extensions_are_supported = checkDeviceExtensionSupport(device);

	bool swap_chain_is_sufficient = false;
	if (extensions_are_supported)
	{
		SwapChainSupportDetails swap_chain_support = querySwapChainSupport(device);
		swap_chain_is_sufficient = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
	}
	return indices.isComplete() && extensions_are_supported && swap_chain_is_sufficient;
}


bool VulkanManager::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t num_extensions;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &num_extensions, nullptr);

	std::vector<VkExtensionProperties> available_extensions(num_extensions);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &num_extensions, available_extensions.data());

	std::set<std::string> required_extensions(device_extensions_.begin(), device_extensions_.end());
	for (const auto& extension : available_extensions)
	{
		required_extensions.erase(extension.extensionName);
	}
	return required_extensions.empty();
}


VkDevice VulkanManager::createLogicalDevice(VkPhysicalDevice physical_device)
{
	QueueFamilyIndices indices = findQueueFamilies(physical_device);

	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	std::set<uint32_t> unique_queue_families = {indices.graphics_family.value(), indices.present_family.value()};

	float queue_priority = 1.0f;
	for (uint32_t queue_family : unique_queue_families)
	{
		VkDeviceQueueCreateInfo queue_create_info{};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = queue_family;
		queue_create_info.queueCount = 1;
		queue_create_info.pQueuePriorities = &queue_priority;

		queue_create_infos.push_back(queue_create_info);
	}

	VkPhysicalDeviceFeatures device_features{};

	VkDeviceCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	create_info.pQueueCreateInfos = queue_create_infos.data();
	create_info.pEnabledFeatures = &device_features;
	create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions_.size());
	create_info.ppEnabledExtensionNames = device_extensions_.data();
	create_info.enabledLayerCount = 0;

	VkDevice logical_device = VK_NULL_HANDLE;
	if (vkCreateDevice(physical_device, &create_info, nullptr, &logical_device) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create logical device");
	}

	vkGetDeviceQueue(logical_device, indices.graphics_family.value(), 0, &graphics_queue_);
	vkGetDeviceQueue(logical_device, indices.present_family.value(), 0, &present_queue_);
	vkGetDeviceQueue(logical_device, indices.compute_family.value(), 0, &compute_queue_);

	return logical_device;
}


void VulkanManager::createSwapChain()
{
	SwapChainSupportDetails swap_chain_support = querySwapChainSupport(device_.physical);

	VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(swap_chain_support.formats);
	VkPresentModeKHR present_mode = chooseSwapPresentMode(swap_chain_support.present_modes);
	VkExtent2D extent = chooseSwapExtent(swap_chain_support.capabilities);

	// Number of frames to store in the swap chain
	uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
	if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount) {
		image_count = swap_chain_support.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = surface_;
	create_info.minImageCount = image_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = findQueueFamilies(device_.physical);
	uint32_t queue_family_indices[] = {indices.graphics_family.value(), indices.present_family.value()};

	if (indices.graphics_family != indices.present_family)
	{
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = queue_family_indices;
	}
	else
	{
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0; // Optional
		create_info.pQueueFamilyIndices = nullptr; // Optional
	}

	create_info.preTransform = swap_chain_support.capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;

	VkSwapchainKHR swap_chain;
	if (vkCreateSwapchainKHR(device_.logical, &create_info, nullptr, &swap_chain) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create swapchain");
	}

	vkGetSwapchainImagesKHR(device_.logical, swap_chain, &image_count, nullptr);
	swap_chain_images_.resize(image_count);
	vkGetSwapchainImagesKHR(device_.logical, swap_chain, &image_count, swap_chain_images_.data());

	/*
	swap_chain_image_format_ = surface_format.format;
	swap_chain_extent_ = extent;
	*/
	swap_chain_ = SwapChain(device_, swap_chain, surface_format.format, extent);

	return;
}


void VulkanManager::createImageViews()
{
	swap_chain_image_views_.resize(swap_chain_images_.size());
	for (size_t i = 0; i < swap_chain_images_.size(); i++)
	{
		VkImageViewCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = swap_chain_images_[i];
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = swap_chain_.format();
		create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = 1;
		if (vkCreateImageView(device_.logical, &create_info, nullptr, &swap_chain_image_views_[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create image view");
		}
	}
}


void VulkanManager::createRenderPass()
{
	render_pass_ = RenderPass(device_, "main");
	render_pass_.updateSwapChain(swap_chain_);
	render_pass_.addBuffer(raymarched_ssbo_);
	render_pass_.init();
}


void VulkanManager::createGraphicsPipeline()
{
	/*
	graphics_pipeline_ = new GraphicsPipeline(device_);
	graphics_pipeline_->linkToRenderPass(render_pass_, 0);
	graphics_pipeline_->create();
	*/
	return;
}


void VulkanManager::createFramebuffers()
{
	swap_chain_framebuffers_.resize(swap_chain_image_views_.size());
	for (size_t i = 0; i < swap_chain_image_views_.size(); i++)
	{
		VkImageView attachments[] = { swap_chain_image_views_[i] };
		VkFramebufferCreateInfo framebuffer_info{};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = render_pass_.data();
		framebuffer_info.attachmentCount = 1;
		framebuffer_info.pAttachments = attachments;
		framebuffer_info.width = swap_chain_.extent().width;
		framebuffer_info.height = swap_chain_.extent().height;
		framebuffer_info.layers = 1;
		if (vkCreateFramebuffer(device_.logical, &framebuffer_info, nullptr, &swap_chain_framebuffers_[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create framebuffer");
		}
	}
	return;
}


void VulkanManager::createCommandPool()
{
	QueueFamilyIndices queue_family_indices = findQueueFamilies(device_.physical);
	VkCommandPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();
	if (vkCreateCommandPool(device_.logical, &pool_info, nullptr, &command_pool_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create command pool");
	}
}


void VulkanManager::createCommandBuffer()
{
	VkCommandBufferAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = command_pool_;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = 1;
	if (vkAllocateCommandBuffers(device_.logical, &alloc_info, &command_buffer_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate command buffers");
	}
}


void VulkanManager::createSyncObjects()
{
	VkSemaphoreCreateInfo semaphore_info{};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkFenceCreateInfo fence_info{};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Begin with the fence activated to avoid hang on startup

	if (vkCreateSemaphore(device_.logical, &semaphore_info, nullptr, &image_available_semaphore_) != VK_SUCCESS ||
		vkCreateSemaphore(device_.logical, &semaphore_info, nullptr, &render_finished_semaphore_) != VK_SUCCESS ||
		vkCreateFence(device_.logical, &fence_info, nullptr, &in_flight_fence_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create sync objects");
	}

}


void VulkanManager::destroySwapChain()
{
	for (auto framebuffer : swap_chain_framebuffers_)
	{
		vkDestroyFramebuffer(device_.logical, framebuffer, nullptr);
	}
	for (auto image_view : swap_chain_image_views_)
	{
		vkDestroyImageView(device_.logical, image_view, nullptr);
	}
	swap_chain_.destroy();
	//vkDestroySwapchainKHR(device_.logical, swap_chain_, nullptr);

	return;
}


void VulkanManager::recreateSwapChain()
{
	vkDeviceWaitIdle(device_.logical);

	destroySwapChain();

	createSwapChain();
	render_pass_.updateSwapChain(swap_chain_);
	createImageViews();
	createFramebuffers();
}


VkSurfaceFormatKHR VulkanManager::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats)
{
	for (const auto& available_format : available_formats)
	{
		if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return available_format;
		}
	}
	return available_formats[0];
}


VkPresentModeKHR VulkanManager::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes)
{
	for (const auto& available_present_mode : available_present_modes)
	{
		if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return available_present_mode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanManager::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}

	int width, height;
	glfwGetFramebufferSize(window_, &width, &height);

	VkExtent2D actualExtent = {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height)
	};

	actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	return actualExtent;
}


std::vector<const char*> VulkanManager::getRequiredInstanceExtensions()
{
	uint32_t glfw_extension_count = 0;
	const char** glfw_extensions;
	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

	if (enable_validation_layers_)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}



VulkanManager::QueueFamilyIndices VulkanManager::findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

	int i = 0;
	for (const auto& queue_family : queue_families)
	{
		if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			if (!indices.graphics_family.has_value())
			{
				indices.graphics_family = i;
			}
		}
		VkBool32 present_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &present_support);
		if (present_support)
		{
			indices.present_family = i;
		}
		if (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT)
		{
			if (!indices.graphics_family.has_value() || indices.graphics_family != i)
			{
				indices.compute_family = i;
			}
		}
		if (indices.isComplete())
			break;
		i++;
	}
	return indices;
}


VulkanManager::SwapChainSupportDetails VulkanManager::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, nullptr);
	if (format_count != 0)
	{
		details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, details.formats.data());
	}

	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, nullptr);
	if (present_mode_count != 0)
	{
		details.present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, details.present_modes.data());
	}

	return details;
}


bool VulkanManager::checkValidationLayerSupport()
{
	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
	std::vector<VkLayerProperties> available_layers(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

	for (const char* layer_name : validation_layers_)
	{
		bool layer_found = false;
		for (const auto& layer_properties : available_layers)
		{
			if (!strcmp(layer_name, layer_properties.layerName))
			{
				layer_found = true;
				break;
			}
		}
		if (!layer_found)
		{
			return false;
		}
	}
	return true;
}


void VulkanManager::createComputeShader()
{
	compute_shader_manager_ = ComputeShaderManager(device_, std::string(xstr(SHADER_DIRECTORY)) + "mainc.spv");
	compute_shader_manager_.addBuffer(world_ssbo_);
	compute_shader_manager_.addBuffer(raymarched_ssbo_);
	compute_shader_manager_.init();

	return;
}


void VulkanManager::createComputeCommandPool()
{
	QueueFamilyIndices queue_family_indices = findQueueFamilies(device_.physical);
	VkCommandPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex = queue_family_indices.compute_family.value();
	if (vkCreateCommandPool(device_.logical, &pool_info, nullptr, &compute_command_pool_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create compute command pool");
	}
}


void VulkanManager::createComputeCommandBuffer()
{
	VkCommandBufferAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = compute_command_pool_;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = 1;
	if (vkAllocateCommandBuffers(device_.logical, &alloc_info, &compute_command_buffer_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate compute command buffers");
	}
}


void VulkanManager::createBuffers()
{
	world_ssbo_ = Buffer(device_,
			4,
			Buffer::STORAGE_TYPE,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			);
	raymarched_ssbo_ = Buffer(device_,
			4*4,
			Buffer::STORAGE_TYPE,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			);
}


void VulkanManager::framebufferSizeCallback(GLFWwindow *window, int width, int height)
{
	auto vulkan_manager = reinterpret_cast<VulkanManager*>(glfwGetWindowUserPointer(window));
	vulkan_manager->framebuffer_resized_ = true;
	vulkan_manager->window_width_ = width;
	vulkan_manager->window_height_ = height;
}

} // namespace Anthrax
