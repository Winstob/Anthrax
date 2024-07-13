/* ---------------------------------------------------------------- *\
 * buffer.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-07-13
\* ---------------------------------------------------------------- */


#ifndef BUFFER_HPP
#define BUFFER_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


namespace Anthrax
{


class Buffer
{

public:
  Buffer(VkDevice device_, VkPhysicalDevice physical_device_);
  ~Buffer();

private:
  void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &buffer_memory);
  uint32_t findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties);

  VkDevice device_;
  VkPhysicalDevice physical_device_;
  VkBuffer buffer_;

};

} // namespace Anthrax

#endif // BUFFER_HPP
