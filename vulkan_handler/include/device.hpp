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

#include <optional>
#include <vector>
#include <set>
#include <fstream>


namespace Anthrax
{

class Device
{
public:
	/*
	Device(VkPhysicalDevice physical_device, VkDevice logical_device);
	Device() : Device(VK_NULL_HANDLE, VK_NULL_HANDLE) {};
	*/
	Device(VkInstance instance, VkSurfaceKHR surface);
	Device() {}
	~Device();
	//Device& operator=(const Device &rhs_device);

	void destroy();

	class QueueFamilyIndices
	{
	public:
		std::optional<uint32_t> graphics_family;
		std::optional<uint32_t> present_family;
		std::optional<uint32_t> compute_family;
		std::optional<uint32_t> transfer_family;
		bool isComplete()
		{
			return graphics_family.has_value() &&
				present_family.has_value() &&
				compute_family.has_value() &&
				transfer_family.has_value();
		}
	};
	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> present_modes;
	};
	SwapChainSupportDetails querySwapChainSupport() { return querySwapChainSupport(physical); }

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer command_buffer);
	QueueFamilyIndices getQueueFamilyIndices() { return queue_family_indices_; }
	VkQueue getGraphicsQueue() { return graphics_queue_; }
	VkQueue getPresentQueue() { return present_queue_; }
	VkQueue getComputeQueue() { return compute_queue_; }
	VkQueue getTransferQueue() { return transfer_queue_; }
	VkCommandPool getTransferCommandPool() { return transfer_command_pool_; }

	VkDevice logical;
	VkPhysicalDevice physical;

private:
	VkPhysicalDevice pickPhysicalDevice();
	VkDevice createLogicalDevice(VkPhysicalDevice physical_device);
	bool isDeviceSuitable(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	QueueFamilyIndices queue_family_indices_;

	void createCommandPool();
	VkCommandPool command_pool_;
	void createTransferCommandPool();
	VkCommandPool transfer_command_pool_;

	VkQueue graphics_queue_;
	VkQueue present_queue_;
	VkQueue compute_queue_;
	VkQueue transfer_queue_;

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

	VkInstance instance_;
	VkSurfaceKHR surface_;

	std::vector<const char*> device_extensions_ = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

};

} // namesapce Anthrax

#endif // DEVICE_HPP
