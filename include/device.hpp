/* ---------------------------------------------------------------- *\
 * device.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-07-13
\* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- *\
 * This is a container for a physical/logical vulkan device pair.
 * The device needs to be manually destroyed when no longer in use.
\* ---------------------------------------------------------------- */


#ifndef DEVICE_HPP
#define DEVICE_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


namespace Anthrax
{

class Device
{
public:
	Device(VkPhysicalDevice physical_device, VkDevice logical_device);
	Device() : Device(VK_NULL_HANDLE, VK_NULL_HANDLE) {};
	~Device();
	Device& operator=(const Device &rhs_device);

	void destroy();

	VkDevice logical;
	VkPhysicalDevice physical;

private:

};

} // namesapce Anthrax

#endif // DEVICE_HPP
