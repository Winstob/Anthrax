/* ---------------------------------------------------------------- *\
 * device.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-07-13
\* ---------------------------------------------------------------- */

#include "device.hpp"

namespace Anthrax
{


Device::Device(VkPhysicalDevice physical_device, VkDevice logical_device)
{
  physical = physical_device;
  logical = logical_device;
}


Device::~Device()
{
}


Device& Device::operator=(const Device &rhs_device)
{
	logical = rhs_device.logical;
	physical = rhs_device.physical;
	return *this;
}

void Device::destroy()
{
	vkDestroyDevice(logical, nullptr);
}


} // namespace Anthrax
