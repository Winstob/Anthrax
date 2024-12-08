/* ---------------------------------------------------------------- *\
 * swap_chain.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-08-05
\* ---------------------------------------------------------------- */

#include "swap_chain.hpp"

namespace Anthrax
{

SwapChain::SwapChain(Device device, VkSwapchainKHR swap_chain, VkFormat format, VkExtent2D extent)
{
	device_ = device;

	swap_chain_ = swap_chain;
	format_ = format;
	extent_ = extent;

	return;
}


SwapChain::~SwapChain()
{
	return;
}


void SwapChain::destroy()
{
	vkDestroySwapchainKHR(device_.logical, swap_chain_, nullptr);
}

} // namespace Anthrax
