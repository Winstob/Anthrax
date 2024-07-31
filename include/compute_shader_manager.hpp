/* ---------------------------------------------------------------- *\
 * compute_shader_manager.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-07-18
\* ---------------------------------------------------------------- */

#ifndef COMPUTE_SHADER_MANAGER_HPP
#define COMPUTE_SHADER_MANAGER_HPP

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

namespace Anthrax
{

class ComputeShaderManager
{
public:
	ComputeShaderManager(Device device, std::string shadercode_filename);
	ComputeShaderManager() {};
	~ComputeShaderManager();
	void destroy();

	void init();
	void addBuffer(Buffer buffer);

	void recordCommandBuffer(VkCommandBuffer command_buffer);

private:
	Device device_;

	std::vector<Buffer> buffers_;
	std::string shadercode_filename_;

	VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
	VkDescriptorSet descriptor_set_ = VK_NULL_HANDLE;
	VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
	VkPipeline pipeline_ = VK_NULL_HANDLE;

};

} // namespace Anthrax
#endif // COMPUTE_SHADER_MANAGER_HPP
