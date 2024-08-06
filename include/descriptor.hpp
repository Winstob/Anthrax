/* ---------------------------------------------------------------- *\
 * descriptor.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-08-06
\* ---------------------------------------------------------------- */

#ifndef DESCRIPTOR_HPP
#define DESCRIPTOR_HPP

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

namespace Anthrax
{

class Descriptor
{
public:
	enum ShaderStage
	{
		VERTEX,
		GEOMETRY,
		FRAGMENT,
		COMPUTE
	};
	Descriptor(Device device, ShaderStage stage, std::vector<Buffer> buffers, std::vector<Image> images);
	Descriptor() {};
	~Descriptor();
	void destroy();

	void init();

	VkDescriptorSetLayout *getDescriptorSetLayoutPtr() { return &descriptor_set_layout_; }
	VkDescriptorSet *getDescriptorSetPtr() { return &descriptor_set_; }

private:
	Device device_;

	std::vector<Buffer> buffers_;
	std::string shadercode_filename_;

	VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
	VkDescriptorSet descriptor_set_ = VK_NULL_HANDLE;

};

} // namespace Anthrax

#endif // DESCRIPTOR_HPP
