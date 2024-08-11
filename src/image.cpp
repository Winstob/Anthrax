/* ---------------------------------------------------------------- *\
 * image.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-08-05
\* ---------------------------------------------------------------- */

#include "image.hpp"

#include <fstream>


namespace Anthrax
{

Image::Image(Device device, int width, int height, VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
{
	device_ = device;

	VkImageCreateInfo image_create_info{};
	image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.imageType = VK_IMAGE_TYPE_2D;
	image_create_info.extent.width = width;
	image_create_info.extent.height = height;
	image_create_info.extent.depth = 1;
	image_create_info.mipLevels = 1;
	image_create_info.arrayLayers = 1;
	image_create_info.format = format_;
	image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
	image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_create_info.usage = usage;
	image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(device_.logical, &image_create_info, nullptr, &image_) != VK_SUCCESS)
	{
		throw std::runtime_error("Error: Failed to create image!");
	}

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(device_.logical, image_, &memory_requirements);

	VkMemoryAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = memory_requirements.size;
	alloc_info.memoryTypeIndex = findMemoryType(memory_requirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device_.logical, &alloc_info, nullptr, &image_memory_) != VK_SUCCESS)
	{
		throw std::runtime_error("Error: Failed to allocate image memory!");
	}

	vkBindImageMemory(device_.logical, image_, image_memory_, 0);
	return;
}


Image::~Image()
{
	//destroy();

	return;
}


void Image::destroy()
{
	vkDestroyImageView(device_.logical, image_view_, nullptr);
	vkDestroyImage(device_.logical, image_, nullptr);
	vkFreeMemory(device_.logical, image_memory_, nullptr);

	return;
}


uint32_t Image::findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties)
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


void Image::createImageView()
{
	VkImageViewCreateInfo image_view_create_info{};
	image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_create_info.image = image_;
	image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	image_view_create_info.format = format_;
	image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_view_create_info.subresourceRange.baseMipLevel = 0;
	image_view_create_info.subresourceRange.levelCount = 1;
	image_view_create_info.subresourceRange.baseArrayLayer = 0;
	image_view_create_info.subresourceRange.layerCount = 1;

	if (vkCreateImageView(device_.logical, &image_view_create_info, nullptr, &image_view_) != VK_SUCCESS)
	{
		throw std::runtime_error("Error: Failed to create image view!");
	}

	return;
}


} // namespace Anthrax
