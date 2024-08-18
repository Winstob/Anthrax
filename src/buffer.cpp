/* ---------------------------------------------------------------- *\
 * buffer.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-07-13
\* ---------------------------------------------------------------- */

#include "buffer.hpp"

#include <fstream>


namespace Anthrax
{


Buffer::Buffer(Device device, size_t size, BufferType buffer_type, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
	device_ = device;
	type_ = buffer_type;
	size_ = size;

	VkBufferUsageFlags type_usage_flag;
	switch (type_)
	{
		case STORAGE_TYPE:
			type_usage_flag = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			break;
		case UNIFORM_TYPE:
			type_usage_flag = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			break;
	}

	VkBufferCreateInfo buffer_info{};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = (VkDeviceSize)size;
	buffer_info.usage = type_usage_flag | usage;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device_.logical, &buffer_info, nullptr, &buffer_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create buffer!");
	}

	VkMemoryRequirements mem_requirements;
	vkGetBufferMemoryRequirements(device_.logical, buffer_, &mem_requirements);


	VkMemoryAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_requirements.size;
	alloc_info.memoryTypeIndex = findMemoryType(mem_requirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device_.logical, &alloc_info, nullptr, &buffer_memory_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate buffer memory!");
	}

	vkBindBufferMemory(device_.logical, buffer_, buffer_memory_, 0);

	//if (type_ == UNIFORM_TYPE)
	if (true)
	{
		vkMapMemory(device_.logical, buffer_memory_, 0, size_, 0, &buffer_memory_mapped_);
	}
	return;
}


Buffer::~Buffer()
{
	//destroy();

	return;
}


void Buffer::destroy()
{
	vkDestroyBuffer(device_.logical, buffer_, nullptr);
	vkFreeMemory(device_.logical, buffer_memory_, nullptr);

	return;
}


uint32_t Buffer::findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties mem_properties;
	vkGetPhysicalDeviceMemoryProperties(device_.physical, &mem_properties);

	for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
	{
		if (type_filter & (1 << i) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	throw std::runtime_error("Failed to find suitable memory type!");
}


void* Buffer::getMappedPtr()
{
	//if (type_ != UNIFORM_TYPE)
	if (false)
	{
		throw std::runtime_error("getMappedPtr() was called from a non-uniform buffer");
	}
	return buffer_memory_mapped_;
}


} // namespace Anthrax
