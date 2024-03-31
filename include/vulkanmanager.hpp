/* ---------------------------------------------------------------- *\
 * vulkanmanager.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-03-26
\* ---------------------------------------------------------------- */

#ifndef VULKANMANAGER_HPP
#define VULKANMANAGER_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <set>
#include <string>
#include <optional>
#include <fstream>

namespace Anthrax
{

class VulkanManager
{
public:
  VulkanManager();
  ~VulkanManager();
  void init();
  void drawFrame();

private:
  void destroy();

  void createInstance();
  void createSurface();
  void pickPhysicalDevice();
  bool isDeviceSuitable(VkPhysicalDevice device);
  bool checkDeviceExtensionSupport(VkPhysicalDevice device);
  void createLogicalDevice();
  void createSwapChain();
  void createImageViews();
  void createRenderPass();
  void createGraphicsPipeline();
  void createFramebuffers();
  void createCommandPool();
  void createCommandBuffer();
  void createSyncObjects();

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);
  VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

  std::vector<const char*> getRequiredInstanceExtensions();

  std::vector<const char*> device_extensions_ = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };

  static GLFWwindow *window_;
  VkSurfaceKHR surface_;
  VkInstance instance_;
  VkPhysicalDevice physical_device_;
  VkDevice device_;

  VkSwapchainKHR swap_chain_;
  std::vector<VkImage> swap_chain_images_;
  VkFormat swap_chain_image_format_;
  VkExtent2D swap_chain_extent_;
  std::vector<VkImageView> swap_chain_image_views_;
  std::vector<VkFramebuffer> swap_chain_framebuffers_;

  VkRenderPass render_pass_;
  VkPipelineLayout pipeline_layout_;
  VkPipeline graphics_pipeline_;

  VkCommandPool command_pool_;
  VkCommandBuffer command_buffer_;
  void recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index);

  VkSemaphore image_available_semaphore_;
  VkSemaphore render_finished_semaphore_;
  VkFence in_flight_fence_;

  // Settings
  static unsigned int window_width_, window_height_;


  class QueueFamilyIndices
  {
  public:
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;
    bool isComplete() { return graphics_family.has_value() && present_family.has_value(); }
  };
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
  VkQueue graphics_queue_;
  VkQueue present_queue_;

  struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
  };
  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

  static std::vector<char> readFile(const std::string &filename);
  VkShaderModule createShaderModule(const std::vector<char> &code);

  // Validation layers
  const std::vector<const char*> validation_layers_ = {
    "VK_LAYER_KHRONOS_validation"
  };
  const bool enable_validation_layers_ = true;
  bool checkValidationLayerSupport();
};

} // namespace Anthrax
#endif // VULKANMANAGER_HPP
