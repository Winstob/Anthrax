/* ---------------------------------------------------------------- *\
 * render_pass.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-08-04
\* ---------------------------------------------------------------- */

#ifndef RENDER_PASS_HPP
#define RENDER_PASS_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <set>
#include <string>
#include <optional>
#include <array>
#include <fstream>

#include "tools.hpp"
#include "device.hpp"
#include "buffer.hpp"
#include "shader.hpp"
#include "graphics_pipeline.hpp"
#include "swap_chain.hpp"

namespace Anthrax
{

class RenderPass
{
public:
	RenderPass(Device device, std::string shadercode_filename);
	RenderPass() {};
	~RenderPass();
	void destroy();

	VkRenderPass data() { return render_pass_; }

	void updateSwapChain(SwapChain swap_chain) { swap_chain_ = swap_chain; } // TODO: make swapchain a pointer to an object shared between everything so this isn't necessary
	void init();
	void addBuffer(Buffer buffer) { buffers_.push_back(buffer); }
	void addImage(Image image) { pipeline_.addImage(image); } // TODO: fix (see buffers_)

	void recordCommandBuffer(VkCommandBuffer command_buffer, VkFramebuffer framebuffer);

private:
	Device device_;

	std::vector<Buffer> buffers_;
	std::string shadercode_file_prefix_;

	VkRenderPass render_pass_;

	VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
	VkDescriptorSet descriptor_set_ = VK_NULL_HANDLE;
	GraphicsPipeline pipeline_;

	SwapChain swap_chain_;

};

} // namespace Anthrax

#endif // RENDER_PASS_HPP
