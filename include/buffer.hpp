/* ---------------------------------------------------------------- *\
 * buffer.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-07-13
\* ---------------------------------------------------------------- */


#ifndef BUFFER_HPP
#define BUFFER_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "device.hpp"

namespace Anthrax
{


class Buffer
{

public:
	Buffer(Device device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	~Buffer();

	VkBuffer data() { return buffer_; }
	VkDeviceMemory getMemoryPtr() { return buffer_memory_; }

private:
	uint32_t findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties);

	Device device_;
	VkBuffer buffer_;
	VkDeviceMemory buffer_memory_;

};

} // namespace Anthrax

#endif // BUFFER_HPP
