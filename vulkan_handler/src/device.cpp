/* ---------------------------------------------------------------- *\
 * device.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-07-13
\* ---------------------------------------------------------------- */

#include "device.hpp"

namespace Anthrax
{


//Device::Device(VkPhysicalDevice physical_device, VkDevice logical_device)
Device::Device(VkInstance instance, VkSurfaceKHR surface)
{
	instance_ = instance;
	surface_ = surface;

	physical = pickPhysicalDevice();
	logical = createLogicalDevice(physical);
	queue_family_indices_ = findQueueFamilies(physical);

	createCommandPool();
	createComputeCommandPool();
	createTransferCommandPool();
}


Device::~Device()
{
}


/*
Device& Device::operator=(const Device &rhs_device)
{
	logical = rhs_device.logical;
	physical = rhs_device.physical;
	return *this;
}
*/

void Device::destroy()
{
	vkDestroyCommandPool(logical, command_pool_, nullptr);
	vkDestroyCommandPool(logical, compute_command_pool_, nullptr);
	vkDestroyCommandPool(logical, transfer_command_pool_, nullptr);
	vkDestroyDevice(logical, nullptr);
	return;
}


VkPhysicalDevice Device::pickPhysicalDevice()
{
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	std::vector<VkPhysicalDevice> suitable_devices;
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);
	if (device_count == 0)
		throw std::runtime_error("No compatible GPUs found");

	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());

	for (const auto &device : devices)
	{
		VkPhysicalDeviceProperties device_properties;
		vkGetPhysicalDeviceProperties(device, &device_properties);
		//std::cout << device_properties.deviceName << std::endl;
		if (isDeviceSuitable(device))
		{
			suitable_devices.push_back(device);
		}
	}

	if (suitable_devices.size() == 0)
	{
		throw std::runtime_error("Failed to find suitable GPU");
	}

	physical_device = suitable_devices[0];
	return physical_device;
}


VkDevice Device::createLogicalDevice(VkPhysicalDevice physical_device)
{
	QueueFamilyIndices indices = findQueueFamilies(physical_device);

	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	std::set<uint32_t> unique_queue_families = {indices.graphics_family.value(), indices.present_family.value()};

	float queue_priority = 1.0f;
	for (uint32_t queue_family : unique_queue_families)
	{
		VkDeviceQueueCreateInfo queue_create_info{};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = queue_family;
		queue_create_info.queueCount = 1;
		queue_create_info.pQueuePriorities = &queue_priority;

		queue_create_infos.push_back(queue_create_info);
	}

	VkPhysicalDeviceFeatures device_features{};
	//device_features.shaderFloat64 = VK_TRUE;
	//device_features.shaderInt64 = VK_TRUE;

	VkDeviceCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	create_info.pQueueCreateInfos = queue_create_infos.data();
	create_info.pEnabledFeatures = &device_features;
	create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions_.size());
	create_info.ppEnabledExtensionNames = device_extensions_.data();
	create_info.enabledLayerCount = 0;

	VkDevice logical_device = VK_NULL_HANDLE;
	if (vkCreateDevice(physical_device, &create_info, nullptr, &logical_device) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create logical device");
	}

	vkGetDeviceQueue(logical_device, indices.graphics_family.value(), 0, &graphics_queue_);
	vkGetDeviceQueue(logical_device, indices.present_family.value(), 0, &present_queue_);
	vkGetDeviceQueue(logical_device, indices.compute_family.value(), 0, &compute_queue_);
	vkGetDeviceQueue(logical_device, indices.transfer_family.value(), 0, &transfer_queue_);

	return logical_device;
}


bool Device::isDeviceSuitable(VkPhysicalDevice device)
{
	/*
	VkPhysicalDeviceProperties device_properties;
	VkPhysicalDeviceFeatures device_features;
	vkGetPhysicalDeviceProperties(device, &device_properties);
	vkGetPhysicalDeviceFeatures(device, &device_features);

	return device_features.geometryShader;
	*/
	QueueFamilyIndices indices = findQueueFamilies(device);
	bool extensions_are_supported = checkDeviceExtensionSupport(device);

	bool swap_chain_is_sufficient = false;
	if (extensions_are_supported)
	{
		SwapChainSupportDetails swap_chain_support = querySwapChainSupport(device);
		swap_chain_is_sufficient = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty() && (swap_chain_support.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT);
	}
	return indices.isComplete() && extensions_are_supported && swap_chain_is_sufficient;
}


bool Device::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t num_extensions;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &num_extensions, nullptr);

	std::vector<VkExtensionProperties> available_extensions(num_extensions);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &num_extensions, available_extensions.data());

	std::set<std::string> required_extensions(device_extensions_.begin(), device_extensions_.end());
	for (const auto& extension : available_extensions)
	{
		required_extensions.erase(extension.extensionName);
	}
	return required_extensions.empty();
}


Device::SwapChainSupportDetails Device::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, nullptr);
	if (format_count != 0)
	{
		details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, details.formats.data());
	}

	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, nullptr);
	if (present_mode_count != 0)
	{
		details.present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, details.present_modes.data());
	}

	return details;
}


VkCommandBuffer Device::beginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = command_pool_;
	alloc_info.commandBufferCount = 1;

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(logical, &alloc_info, &command_buffer);

	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(command_buffer, &begin_info);

	return command_buffer;
}


void Device::endSingleTimeCommands(VkCommandBuffer command_buffer)
{
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;

	vkQueueSubmit(graphics_queue_, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphics_queue_);

	vkFreeCommandBuffers(logical, command_pool_, 1, &command_buffer);
	return;
}


Device::QueueFamilyIndices Device::findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

	int i = 0;
	for (const auto& queue_family : queue_families)
	{
		if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			if (!indices.graphics_family.has_value())
			{
				indices.graphics_family = i;
			}
		}
		VkBool32 present_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &present_support);
		bool is_used = false;
		if (present_support)
		{
			indices.present_family = i;
			is_used = true;
		}
		if (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT)
		{
			if (!indices.compute_family.has_value() || !is_used)
			{
				indices.compute_family = i;
				is_used = true;
			}
		}
		if (queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT)
		{
			if (!indices.transfer_family.has_value() || !is_used)
			{
				indices.transfer_family = i;
				is_used = true;
			}
		}
		if (indices.isComplete())
			break;
		i++;
	}
	return indices;
}


void Device::createCommandPool()
{
	VkCommandPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex = queue_family_indices_.graphics_family.value();
	if (vkCreateCommandPool(logical, &pool_info, nullptr, &command_pool_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create command pool");
	}
}


void Device::createComputeCommandPool()
{
	VkCommandPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex = queue_family_indices_.compute_family.value();
	if (vkCreateCommandPool(logical, &pool_info, nullptr, &compute_command_pool_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create compute command pool");
	}
}


void Device::createTransferCommandPool()
{
	VkCommandPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	pool_info.queueFamilyIndex = queue_family_indices_.transfer_family.value();
	if (vkCreateCommandPool(logical, &pool_info, nullptr, &transfer_command_pool_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create transfer command pool");
	}
}


/**
 * @brief Create a new command pool.
 *
 * @param type The command type.
 *
 * @return The newly created command pool. Must be cleaned up with vkDestroyCommandPool().
 */
VkCommandPool Device::newCommandPool(CommandType type)
{
	VkCommandPool new_command_pool;

	VkCommandPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	switch (type)
	{
		case CommandType::GRAPHICS:
			pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			pool_info.queueFamilyIndex = queue_family_indices_.graphics_family.value();
			break;
		case CommandType::COMPUTE:
			pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			pool_info.queueFamilyIndex = queue_family_indices_.compute_family.value();
			break;
		case CommandType::TRANSFER:
			pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
			pool_info.queueFamilyIndex = queue_family_indices_.transfer_family.value();
			break;
	}
	if (vkCreateCommandPool(logical, &pool_info, nullptr, &new_command_pool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create command pool");
	}

	return new_command_pool;
}

} // namespace Anthrax
