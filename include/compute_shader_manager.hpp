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
#include "image.hpp"
#include "shader.hpp"
#include "descriptor.hpp"

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
	void addBuffer(Buffer buffer) { buffers_.push_back(buffer); }
	void addImage(Image image) { images_.push_back(image); }

	void recordCommandBuffer(VkCommandBuffer command_buffer, unsigned int x_work_groups, unsigned int y_work_groups, unsigned int z_work_groups);

private:
	Device device_;

	std::vector<Buffer> buffers_;
	std::vector<Image> images_;
	std::string shadercode_filename_;

	Descriptor descriptor_;
	VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
	VkPipeline pipeline_ = VK_NULL_HANDLE;

};

} // namespace Anthrax
#endif // COMPUTE_SHADER_MANAGER_HPP
