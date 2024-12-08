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

#define WORKGROUP_SIZE 8

namespace Anthrax
{


// Static member allocation
GLFWwindow *VulkanManager::window_;



VulkanManager::VulkanManager()
{
	// default values
	window_width_ = 800;
	window_height_ = 600;
	max_frames_in_flight_ = 1;
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

	input_handler_ = InputHandler(window_);
	input_handler_.captureMouse();

	frames_.resize(max_frames_in_flight_);

	return;
}


void VulkanManager::start()
{
	//createBuffers();
	createRenderPass();
	createComputeShader();
	//addAllBuffers();
	render_pass_.setDescriptors(render_pass_descriptors_);
	compute_shader_manager_.setDescriptors(compute_pass_descriptors_);
	render_pass_.init();
	compute_shader_manager_.init();

	createFramebuffers();

	createCommandPool();
	createCommandBuffer();

	createComputeCommandPool();
	createComputeCommandBuffer();

	createSyncObjects();

	return;
}


void VulkanManager::setMultiBuffering(int max_frames_in_flight)
{
	if (!frames_.empty())
	{
		throw std::runtime_error("Can't set multibuffering after VulkanManager is initialized!");
	}
	if (max_frames_in_flight < 1)
	{
		throw std::runtime_error("Multi-buffering value must be at least 1!");
	}
	max_frames_in_flight_ = max_frames_in_flight;
	return;
}


void VulkanManager::drawFrame()
{
	//if (vkGetFenceStatus(device_.logical, frames_[current_frame_].in_flight_fence) == VK_NOT_READY) std::cout << "Waiting for fence " << current_frame_ << std::endl;
	vkWaitForFences(device_.logical, 1, &(frames_[current_frame_].in_flight_fence), VK_TRUE, UINT64_MAX); // since the render pass waits for compute pass to finish before starting, we only need to wait for the render pass
	vkResetFences(device_.logical, 1, &(frames_[current_frame_].in_flight_fence));

	compute_shader_manager_.selectDescriptor(current_frame_);
	render_pass_.selectDescriptor(current_frame_);

	// Compute pass
	// TODO: compute buffer updates should really go here. Maybe when buffers are moved away from host-visible memory, set them up for transfers normally and initiate the transfer here.
	vkResetCommandBuffer((frames_[current_frame_].compute_command_buffer), 0);
	compute_shader_manager_.recordCommandBuffer(frames_[current_frame_].compute_command_buffer, ceil((float)window_width_/WORKGROUP_SIZE), ceil((float)window_height_/WORKGROUP_SIZE), 1);

	VkSubmitInfo compute_submit_info{};
	compute_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	compute_submit_info.commandBufferCount = 1;
	compute_submit_info.pCommandBuffers = &(frames_[current_frame_].compute_command_buffer);
	compute_submit_info.signalSemaphoreCount = 1;
	compute_submit_info.pSignalSemaphores = &(frames_[current_frame_].compute_finished_semaphore);
	//compute_submit_info.
	
	if (vkQueueSubmit(device_.getComputeQueue(), 1, &compute_submit_info, VK_NULL_HANDLE) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to submit dispatch command buffer");
	}

	// Graphics pass
	uint32_t image_index;
	vkAcquireNextImageKHR(device_.logical, swap_chain_.data(), UINT64_MAX, frames_[current_frame_].image_available_semaphore, VK_NULL_HANDLE, &image_index);

	vkResetCommandBuffer(frames_[current_frame_].command_buffer, 0);
	//recordCommandBuffer(command_buffer_, image_index);
	render_pass_.recordCommandBuffer(frames_[current_frame_].command_buffer, swap_chain_framebuffers_[image_index]);

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore wait_semaphores[] = {frames_[current_frame_].compute_finished_semaphore, frames_[current_frame_].image_available_semaphore};
	VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submit_info.waitSemaphoreCount = 2;
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &(frames_[current_frame_].command_buffer);
	VkSemaphore signal_semaphores[] = {frames_[current_frame_].render_finished_semaphore};
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = signal_semaphores;
	if (vkQueueSubmit(device_.getGraphicsQueue(), 1, &submit_info, frames_[current_frame_].in_flight_fence) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to submit draw command buffer");
	}
	//std::cout << "Submitted frame " << current_frame_ << std::endl;


	VkPresentInfoKHR present_info{};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = signal_semaphores;
	VkSwapchainKHR swap_chains[] = { swap_chain_.data() };
	present_info.swapchainCount = 1;
	present_info.pSwapchains = swap_chains;
	present_info.pImageIndices = &image_index;
	present_info.pResults = nullptr;
	VkResult result = vkQueuePresentKHR(device_.getPresentQueue(), &present_info);
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

	// set up input handler for a new frame
	input_handler_.beginNewFrame();
	input_handler_.getMouseMovementX();
	input_handler_.getMouseMovementY();

	current_frame_ = (current_frame_ + 1) % frames_.size();
	return;
}


void VulkanManager::destroy()
{
	vkDeviceWaitIdle(device_.logical); // Wait for any asynchronous operations to finish

	input_handler_.destroy();

	// Buffers/Images
	/*
	raymarched_image_.destroy();
	world_ssbo_.destroy();
	*/

	// Compute shader stuff
	compute_shader_manager_.destroy();

	for (unsigned int i = 0; i < frames_.size(); i++)
	{
		vkDestroySemaphore(device_.logical, frames_[i].compute_finished_semaphore, nullptr);
		vkDestroySemaphore(device_.logical, frames_[i].image_available_semaphore, nullptr);
		vkDestroySemaphore(device_.logical, frames_[i].render_finished_semaphore, nullptr);
		vkDestroyFence(device_.logical, frames_[i].in_flight_fence, nullptr);
	}
	vkDestroyCommandPool(device_.logical, compute_command_pool_, nullptr);
	vkDestroyCommandPool(device_.logical, command_pool_, nullptr);

	destroySwapChain();

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
	device_ = Device(instance_, surface_);
	return;
}


void VulkanManager::createSwapChain()
{
	Device::SwapChainSupportDetails swap_chain_support = device_.querySwapChainSupport();

	VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(swap_chain_support.formats);
	VkPresentModeKHR present_mode = chooseSwapPresentMode(swap_chain_support.present_modes);
	VkExtent2D extent = chooseSwapExtent(swap_chain_support.capabilities);

	// Number of frames to store in the swap chain
	uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
	if (swap_chain_support.capabilities.maxImageCount > 0 && image_count < swap_chain_support.capabilities.maxImageCount) {
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
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

	//QueueFamilyIndices indices = findQueueFamilies(device_.physical);
	Device::QueueFamilyIndices indices = device_.getQueueFamilyIndices();
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
	/*
	render_pass_.addImage(raymarched_image_);
	*/
	//render_pass_.init();
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
	//QueueFamilyIndices queue_family_indices = findQueueFamilies(device_.physical);
	Device::QueueFamilyIndices queue_family_indices = device_.getQueueFamilyIndices();
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
	for (unsigned int i = 0; i < frames_.size(); i++)
	{
		if (vkAllocateCommandBuffers(device_.logical, &alloc_info, &(frames_[i].command_buffer)) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate command buffers");
		}
	}
}


void VulkanManager::createSyncObjects()
{
	VkSemaphoreCreateInfo semaphore_info{};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkFenceCreateInfo fence_info{};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Begin with the fence activated to avoid hang on startup

	for (unsigned int i = 0; i < frames_.size(); i++)
	{
		// graphics semaphores
		if (vkCreateSemaphore(device_.logical, &semaphore_info, nullptr, &(frames_[i].image_available_semaphore)) != VK_SUCCESS ||
			vkCreateSemaphore(device_.logical, &semaphore_info, nullptr, &(frames_[i].render_finished_semaphore)) != VK_SUCCESS ||
			vkCreateFence(device_.logical, &fence_info, nullptr, &(frames_[i].in_flight_fence)) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create graphics sync objects");
		}

		// compute semaphores
		if (vkCreateSemaphore(device_.logical, &semaphore_info, nullptr, &(frames_[i].compute_finished_semaphore)) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create compute sync objects");
		}
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
		//if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		if (available_format.format == VK_FORMAT_B8G8R8A8_UNORM && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
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
	/*
	compute_shader_manager_.addBuffer(world_ssbo_);
	compute_shader_manager_.addImage(raymarched_image_);
	*/
	//compute_shader_manager_.init();

	return;
}


void VulkanManager::createComputeCommandPool()
{
	//QueueFamilyIndices queue_family_indices = findQueueFamilies(device_.physical);
	Device::QueueFamilyIndices queue_family_indices = device_.getQueueFamilyIndices();
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
	for (unsigned int i = 0; i < frames_.size(); i++)
	{
		if (vkAllocateCommandBuffers(device_.logical, &alloc_info, &(frames_[i].compute_command_buffer)) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate compute command buffers");
		}
	}
}


void VulkanManager::createBuffers()
{
	/*
	world_ssbo_ = Buffer(device_,
			4,
			Buffer::STORAGE_TYPE,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			);
	raymarched_image_ = Image(device_,
			1920,
			1080,
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			);
		*/
}


void VulkanManager::addAllBuffers()
{
	/*
	for (unsigned int i = 0; i < render_pass_buffers_.size(); i++)
	{
		render_pass_.addBuffer(render_pass_buffers_[i]);
	}
	for (unsigned int i = 0; i < render_pass_images_.size(); i++)
	{
		render_pass_.addImage(render_pass_images_[i]);
	}
	for (unsigned int i = 0; i < compute_pass_buffers_.size(); i++)
	{
		compute_shader_manager_.addBuffer(compute_pass_buffers_[i]);
	}
	for (unsigned int i = 0; i < compute_pass_images_.size(); i++)
	{
		compute_shader_manager_.addImage(compute_pass_images_[i]);
	}
	*/
	return;
}


void VulkanManager::framebufferSizeCallback(GLFWwindow *window, int width, int height)
{
	auto vulkan_manager = reinterpret_cast<VulkanManager*>(glfwGetWindowUserPointer(window));
	vulkan_manager->framebuffer_resized_ = true;
	vulkan_manager->window_width_ = width;
	vulkan_manager->window_height_ = height;
}

} // namespace Anthrax
