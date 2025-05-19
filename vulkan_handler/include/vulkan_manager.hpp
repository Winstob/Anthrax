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
#include <math.h>

#include "device.hpp"
#include "render_pass.hpp"
#include "compute_shader_manager.hpp"
#include "swap_chain.hpp"
#include "input_handler.hpp"

namespace Anthrax
{

class VulkanManager
{
public:
	VulkanManager();
	~VulkanManager();
	void init();
	void start();
	//void drawFrame();
	void waitForFences();
	void drawFrame(VkSemaphore *wait_semaphore);

	void setMultiBuffering(int max_frames_in_flight);

	Device getDevice() { return device_; }
	Device *getDevicePtr() { return &device_; }

	int getWindowWidth() { return window_width_; }
	int getWindowHeight() { return window_height_; }

	void renderPassSetDescriptors(std::vector<Descriptor> descriptors) { render_pass_descriptors_ = descriptors; }
	void computePassSetDescriptors(std::vector<Descriptor> descriptors) { compute_pass_descriptors_ = descriptors; }
	void wait() { vkDeviceWaitIdle(device_.logical); }

	GLFWAPI int getKey(int key) { return input_handler_.getKey(key); }
	float getFrameTime() { return input_handler_.getFrameTime(); }
	float getMouseMovementX() { return input_handler_.getMouseMovementX(); }
	float getMouseMovementY() { return input_handler_.getMouseMovementY(); }

	float getFrameTimeAvg() { return input_handler_.getFrameTimeAvg(); }

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

	struct Frame
	{
		VkSemaphore image_available_semaphore;
		VkSemaphore render_finished_semaphore;
		VkFence in_flight_fence;
		VkSemaphore compute_finished_semaphore;
		VkCommandBuffer compute_command_buffer;
		VkCommandBuffer command_buffer;
	};
	//Frame frames_[MAX_FRAMES_IN_FLIGHT];
	std::vector<Frame> frames_;
	int max_frames_in_flight_;
	int current_frame_ = 0;

	/*
	Buffer world_ssbo_, raymarched_ssbo_;
	Image raymarched_image_;
	*/
	std::vector<Buffer> render_pass_buffers_;
	std::vector<Image> render_pass_images_;
	std::vector<Buffer> compute_pass_buffers_;
	std::vector<Image> compute_pass_images_;
	std::vector<Descriptor> render_pass_descriptors_;
	std::vector<Descriptor> compute_pass_descriptors_;

	InputHandler input_handler_;
	float frame_time_ = 0.0;

};

} // namespace Anthrax
#endif // VULKAN_MANAGER_HPP
