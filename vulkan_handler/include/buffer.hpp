/* ---------------------------------------------------------------- *\
 * buffer.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-07-13
\* ---------------------------------------------------------------- */


#ifndef BUFFER_HPP
#define BUFFER_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>

#include "device.hpp"

namespace Anthrax
{

class Buffer
{

public:
	enum BufferType
	{
		UNIFORM_TYPE,
		STORAGE_TYPE
	};

	Buffer(Device device, size_t size, BufferType buffer_type, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	Buffer() {};
	~Buffer();
	void destroy();

	VkBuffer data() { return buffer_; }
	VkDeviceMemory getMemoryPtr() { return buffer_memory_; }
	BufferType type() { return type_; }

	size_t size() { return size_; }
	void* getMappedPtr();

	void copy(Buffer other);

private:
	uint32_t findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties);

	Device device_;
	VkMemoryPropertyFlags properties_;
	VkBuffer buffer_ = VK_NULL_HANDLE;
	VkDeviceMemory buffer_memory_ = VK_NULL_HANDLE;
	void *buffer_memory_mapped_;
	BufferType type_;

	size_t size_; // The size of the buffer in bytes

};

} // namespace Anthrax

#endif // BUFFER_HPP
