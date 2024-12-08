/* ---------------------------------------------------------------- *\
 * swap_chain.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-08-05
\* ---------------------------------------------------------------- */

#ifndef SWAP_CHAIN_HPP
#define SWAP_CHAIN_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <set>
#include <string>
#include <optional>
#include <array>
#include <fstream>

#include "tools.hpp"
#include "device.hpp"

namespace Anthrax
{

class SwapChain
{
public:
	SwapChain() {};
	SwapChain(Device device, VkSwapchainKHR swap_chain, VkFormat format, VkExtent2D extent);
	~SwapChain();
	void destroy();

	VkSwapchainKHR data() { return swap_chain_; }
	VkFormat format() { return format_; }
	VkExtent2D extent() { return extent_; }

private:
	Device device_;

	VkSwapchainKHR swap_chain_;
	VkFormat format_;
	VkExtent2D extent_;

};

} // namespace Anthrax

#endif // SWAP_CHAIN_HPP
