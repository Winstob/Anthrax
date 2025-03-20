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
	properties_ = properties;
	type_ = buffer_type;
	size_ = size;
	buffer_memory_mapped_ = nullptr;

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
	map();

	initialized_ = true;
	return;
}


Buffer::~Buffer()
{
	//destroy();

	return;
}


void Buffer::destroy()
{
	unmap(); // TODO: necessary?
	vkDestroyBuffer(device_.logical, buffer_, nullptr);
	vkFreeMemory(device_.logical, buffer_memory_, nullptr);

	initialized_ = false;

	return;
}


void Buffer::map()
{
	if (buffer_memory_mapped_)
	{
		throw std::runtime_error("Buffer memory already mapped!");
	}
	if (properties_ & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	{
		vkMapMemory(device_.logical, buffer_memory_, 0, size_, 0, &buffer_memory_mapped_);
	}
	return;
}


void Buffer::unmap()
{
	if (buffer_memory_mapped_)
	{
		vkUnmapMemory(device_.logical, buffer_memory_);
		buffer_memory_mapped_ = nullptr;
	}
}


void Buffer::flush()
{
	// TODO: check that the memory is mapped?
	if ((properties_ & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	    && (buffer_memory_mapped_))
	{
		VkMappedMemoryRange flush_range = {};
		flush_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		flush_range.memory = buffer_memory_;
		flush_range.offset = 0;
		flush_range.size = VK_WHOLE_SIZE;
		vkFlushMappedMemoryRanges(device_.logical, 1, &flush_range);
	}
	else
	{
		throw std::runtime_error("Buffer::flush() must be called on a host-visible buffer!");
	}
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
	if (!(properties_ & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
	{
		throw std::runtime_error("getMappedPtr() was called from a local gpu buffer");
	}
	return buffer_memory_mapped_;
}

void Buffer::copy(Buffer other)
{
	// start by creating a new command buffer
	VkCommandBufferAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = device_.getTransferCommandPool();
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = 1;
	VkCommandBuffer transfer_command_buffer;
	if (vkAllocateCommandBuffers(device_.logical, &alloc_info, &transfer_command_buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate transfer command buffer");
	}

	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(transfer_command_buffer, &begin_info);

	VkBufferCopy copy_region{};
	copy_region.srcOffset = 0;
	copy_region.dstOffset = 0;
	copy_region.size = other.size();
	vkCmdCopyBuffer(transfer_command_buffer, other.data(), buffer_, 1, &copy_region);
	vkEndCommandBuffer(transfer_command_buffer);

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &transfer_command_buffer;
	vkQueueSubmit(device_.getTransferQueue(), 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(device_.getTransferQueue());

	vkFreeCommandBuffers(device_.logical, device_.getTransferCommandPool(), 1, &transfer_command_buffer);
	return;
}

} // namespace Anthrax
