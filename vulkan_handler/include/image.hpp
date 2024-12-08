/* ---------------------------------------------------------------- *\
 * image.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-08-05
\* ---------------------------------------------------------------- */


#ifndef IMAGE_HPP
#define IMAGE_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "device.hpp"

namespace Anthrax
{

class Image
{

public:
	Image(Device device, int width, int height, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	Image() {};
	~Image();
	void destroy();

	VkImageView getImageView() { return image_view_; }
	VkImageLayout getImageLayout() { return layout_; }

private:
	uint32_t findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties);
	void createImageView();
	void transitionImageLayout();

	Device device_;
	VkImage image_ = VK_NULL_HANDLE;
	VkDeviceMemory image_memory_ = VK_NULL_HANDLE;
	VkImageView image_view_ = VK_NULL_HANDLE;
	//VkFormat format_ = VK_FORMAT_R8G8B8A8_SRGB;
	//VkFormat format_ = VK_FORMAT_B8G8R8A8_SRGB;
	//VkFormat format_ = VK_FORMAT_D32_SFLOAT;
	VkFormat format_ = VK_FORMAT_R8G8B8A8_UNORM;
	//VkImageLayout layout_ = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageLayout layout_ = VK_IMAGE_LAYOUT_GENERAL;

	size_t size_; // The size of the buffer in bytes

};

} // namespace Anthrax

#endif // IMAGE_HPP
