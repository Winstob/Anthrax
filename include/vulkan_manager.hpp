/* ---------------------------------------------------------------- *\
 * vulkan_manager.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-03-26
\* ---------------------------------------------------------------- */

#ifndef VULKAN_MANAGER_HPP
#define VULKAN_MANAGER_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <set>
#include <string>
#include <optional>
#include <fstream>

#include "device.hpp"
#include "render_pass.hpp"
#include "compute_shader_manager.hpp"
#include "swap_chain.hpp"

namespace Anthrax
{

class VulkanManager
{
public:
	VulkanManager();
	~VulkanManager();
	void init();
	void start();
	void drawFrame();

	Device getDevice() { return device_; }

	int getWindowWidth() { return window_width_; }
	int getWindowHeight() { return window_height_; }

	void renderPassAddBuffer(Buffer buffer) { render_pass_buffers_.push_back(buffer); }
	void renderPassAddImage(Image image) { render_pass_images_.push_back(image); }
	void computePassAddBuffer(Buffer buffer) { compute_pass_buffers_.push_back(buffer); }
	void computePassAddImage(Image image) { compute_pass_images_.push_back(image); }
	void wait() { vkDeviceWaitIdle(device_.logical); }

private:
	void destroy();

	void createInstance();
	void createSurface();
	void createAnthraxDevice();
	void createSwapChain();
	void createImageViews();
	void createRenderPass();
	void createGraphicsPipeline();
	void createFramebuffers();
	void createCommandPool();
	void createCommandBuffer();
	void createSyncObjects();
	void createBuffers();
	void addAllBuffers();

	void destroySwapChain();
	void recreateSwapChain();
	bool framebuffer_resized_ = false;

	void createComputeShader();
	void createComputeCommandPool();
	void createComputeCommandBuffer();

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	std::vector<const char*> getRequiredInstanceExtensions();

	static GLFWwindow *window_;
	VkSurfaceKHR surface_;
	VkInstance instance_;
	Device device_;

	SwapChain swap_chain_;
	/*
	VkSwapchainKHR swap_chain_;
	VkFormat swap_chain_image_format_;
	VkExtent2D swap_chain_extent_;
	*/
	std::vector<VkImage> swap_chain_images_;
	std::vector<VkImageView> swap_chain_image_views_;
	std::vector<VkFramebuffer> swap_chain_framebuffers_;

	//VkRenderPass render_pass_;
	RenderPass render_pass_;
	//GraphicsPipeline *graphics_pipeline_;

	VkCommandPool command_pool_;
	VkCommandBuffer command_buffer_;

	VkSemaphore image_available_semaphore_;
	VkSemaphore render_finished_semaphore_;
	VkFence in_flight_fence_;

	VkSemaphore compute_finished_semaphore_;
	VkFence compute_in_flight_fence_;

	// Settings
	unsigned int window_width_, window_height_;

	// Validation layers
	const std::vector<const char*> validation_layers_ = {
		"VK_LAYER_KHRONOS_validation"
	};
	const bool enable_validation_layers_ = true;
	bool checkValidationLayerSupport();

	// GLFW callbacks
	static void framebufferSizeCallback(GLFWwindow *window, int width, int height);

	ComputeShaderManager compute_shader_manager_;
	VkCommandPool compute_command_pool_;
	VkCommandBuffer compute_command_buffer_;

	/*
	Buffer world_ssbo_, raymarched_ssbo_;
	Image raymarched_image_;
	*/
	std::vector<Buffer> render_pass_buffers_;
	std::vector<Image> render_pass_images_;
	std::vector<Buffer> compute_pass_buffers_;
	std::vector<Image> compute_pass_images_;

};

} // namespace Anthrax
#endif // VULKAN_MANAGER_HPP
