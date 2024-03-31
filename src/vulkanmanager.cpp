/* ---------------------------------------------------------------- *\
 * vulkanmanager.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-03-26
\* ---------------------------------------------------------------- */

#include "vulkanmanager.hpp"

#include <iostream>
#include <algorithm>
#include <limits>
#include <cstring>

#define xstr(s) str(s)
#define str(s) #s

namespace Anthrax
{


// Static member allocation
GLFWwindow *VulkanManager::window_;
unsigned int VulkanManager::window_width_ = 800;
unsigned int VulkanManager::window_height_ = 600;



VulkanManager::VulkanManager()
{
}


VulkanManager::~VulkanManager()
{
  destroy();
}


void VulkanManager::init()
{
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window_ = glfwCreateWindow(window_width_, window_height_, xstr(WINDOW_NAME), NULL, NULL);
  if (window_ == NULL)
  {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return;
  }

  createInstance();
  createSurface();
  pickPhysicalDevice();
  createLogicalDevice();
  createSwapChain();
  createImageViews();
  createRenderPass();
  createGraphicsPipeline();
  createFramebuffers();
  createCommandPool();
  createCommandBuffer();
  createSyncObjects();

}


void VulkanManager::drawFrame()
{
  vkWaitForFences(device_, 1, &in_flight_fence_, VK_TRUE, UINT64_MAX);
  vkResetFences(device_, 1, &in_flight_fence_);

  uint32_t image_index;
  vkAcquireNextImageKHR(device_, swap_chain_, UINT64_MAX, image_available_semaphore_, VK_NULL_HANDLE, &image_index);
  vkResetCommandBuffer(command_buffer_, 0);
  recordCommandBuffer(command_buffer_, image_index);

  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  VkSemaphore wait_semaphores[] = {image_available_semaphore_};
  VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = wait_semaphores;
  submit_info.pWaitDstStageMask = wait_stages;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer_;
  VkSemaphore signal_semaphores[] = {render_finished_semaphore_};
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = signal_semaphores;
  if (vkQueueSubmit(graphics_queue_, 1, &submit_info, in_flight_fence_) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to submit draw command buffer");
  }


  VkPresentInfoKHR present_info{};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = signal_semaphores;
  VkSwapchainKHR swap_chains[] = { swap_chain_ };
  present_info.swapchainCount = 1;
  present_info.pSwapchains = swap_chains;
  present_info.pImageIndices = &image_index;
  present_info.pResults = nullptr;
  vkQueuePresentKHR(present_queue_, &present_info);
}


void VulkanManager::destroy()
{
  vkDeviceWaitIdle(device_); // Wait for any asynchronous operations to finish

  vkDestroySemaphore(device_, image_available_semaphore_, nullptr);
  vkDestroySemaphore(device_, render_finished_semaphore_, nullptr);
  vkDestroyFence(device_, in_flight_fence_, nullptr);
  vkDestroyCommandPool(device_, command_pool_, nullptr);
  for (auto framebuffer : swap_chain_framebuffers_)
  {
    vkDestroyFramebuffer(device_, framebuffer, nullptr);
  }
  vkDestroyPipeline(device_, graphics_pipeline_, nullptr);
  vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
  vkDestroyRenderPass(device_, render_pass_, nullptr);
  for (auto image_view : swap_chain_image_views_)
  {
    vkDestroyImageView(device_, image_view, nullptr);
  }
  vkDestroySwapchainKHR(device_, swap_chain_, nullptr);
  vkDestroyDevice(device_, nullptr);
  vkDestroySurfaceKHR(instance_, surface_, nullptr);
  vkDestroyInstance(instance_, nullptr);
  glfwDestroyWindow(window_);

  glfwTerminate();

  return;
}


void VulkanManager::createInstance()
{
  if (enable_validation_layers_ && !checkValidationLayerSupport())
  {
    throw std::runtime_error("Validation layers not available");
  }

  VkApplicationInfo app_info{};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = xstr(WINDOW_NAME);
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName = "Anthrax";
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;

  if (enable_validation_layers_)
  {
    create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers_.size());
    create_info.ppEnabledLayerNames = validation_layers_.data();
  }
  else
  {
    create_info.enabledLayerCount = 0;
  }

  auto extensions = getRequiredInstanceExtensions();
  create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  create_info.ppEnabledExtensionNames = extensions.data();

  if (vkCreateInstance(&create_info, nullptr, &instance_) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create Vulkan instance");
  }

  return;
}


void VulkanManager::createSurface()
{
  if (glfwCreateWindowSurface(instance_, window_, nullptr, &surface_) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create GLFW window surface");
    glfwTerminate();
  }
  return;
}


void VulkanManager::pickPhysicalDevice()
{
  physical_device_ = VK_NULL_HANDLE;
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

  physical_device_ = suitable_devices[0];
  return;
}


bool VulkanManager::isDeviceSuitable(VkPhysicalDevice device)
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
    swap_chain_is_sufficient = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
  }
  return indices.isComplete() && extensions_are_supported && swap_chain_is_sufficient;
}


bool VulkanManager::checkDeviceExtensionSupport(VkPhysicalDevice device)
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


void VulkanManager::createLogicalDevice()
{
  QueueFamilyIndices indices = findQueueFamilies(physical_device_);

  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  std::set<uint32_t> unique_queue_families = {indices.graphics_family.value(), indices.present_family.value()};

  float queue_priority = 1.0f;
  for (uint32_t queue_family : unique_queue_families)
  {
    VkDeviceQueueCreateInfo queue_create_info{};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = indices.graphics_family.value();
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;

    queue_create_infos.push_back(queue_create_info);
  }

  VkPhysicalDeviceFeatures device_features{};

  VkDeviceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
  create_info.pQueueCreateInfos = queue_create_infos.data();
  create_info.pEnabledFeatures = &device_features;
  create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions_.size());
  create_info.ppEnabledExtensionNames = device_extensions_.data();
  create_info.enabledLayerCount = 0;

  if (vkCreateDevice(physical_device_, &create_info, nullptr, &device_) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create logical device");
  }

  vkGetDeviceQueue(device_, indices.graphics_family.value(), 0, &graphics_queue_);
  vkGetDeviceQueue(device_, indices.present_family.value(), 0, &present_queue_);

  return;
}


void VulkanManager::createSwapChain()
{
  SwapChainSupportDetails swap_chain_support = querySwapChainSupport(physical_device_);

  VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(swap_chain_support.formats);
  VkPresentModeKHR present_mode = chooseSwapPresentMode(swap_chain_support.present_modes);
  VkExtent2D extent = chooseSwapExtent(swap_chain_support.capabilities);

  // Number of frames to store in the swap chain
  uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
  if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount) {
    image_count = swap_chain_support.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.surface = surface_;
  create_info.minImageCount = image_count;
  create_info.imageFormat = surface_format.format;
  create_info.imageColorSpace = surface_format.colorSpace;
  create_info.imageExtent = extent;
  create_info.imageArrayLayers = 1;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices indices = findQueueFamilies(physical_device_);
  uint32_t queue_family_indices[] = {indices.graphics_family.value(), indices.present_family.value()};

  if (indices.graphics_family != indices.present_family)
  {
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = 2;
    create_info.pQueueFamilyIndices = queue_family_indices;
  }
  else
  {
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0; // Optional
    create_info.pQueueFamilyIndices = nullptr; // Optional
  }

  create_info.preTransform = swap_chain_support.capabilities.currentTransform;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_info.presentMode = present_mode;
  create_info.clipped = VK_TRUE;

  if (vkCreateSwapchainKHR(device_, &create_info, nullptr, &swap_chain_) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create swapchain");
  }

  vkGetSwapchainImagesKHR(device_, swap_chain_, &image_count, nullptr);
  swap_chain_images_.resize(image_count);
  vkGetSwapchainImagesKHR(device_, swap_chain_, &image_count, swap_chain_images_.data());

  swap_chain_image_format_ = surface_format.format;
  swap_chain_extent_ = extent;

  return;
}


void VulkanManager::createImageViews()
{
  swap_chain_image_views_.resize(swap_chain_images_.size());
  for (size_t i = 0; i < swap_chain_images_.size(); i++)
  {
    VkImageViewCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = swap_chain_images_[i];
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = swap_chain_image_format_;
    create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device_, &create_info, nullptr, &swap_chain_image_views_[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create image view");
    }
  }
}


void VulkanManager::createRenderPass()
{
  VkAttachmentDescription color_attachment{};
  color_attachment.format = swap_chain_image_format_;
  color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_attachment_ref{};
  color_attachment_ref.attachment = 0;
  color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_attachment_ref;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


  VkRenderPassCreateInfo render_pass_create_info{};
  render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_create_info.attachmentCount = 1;
  render_pass_create_info.pAttachments = &color_attachment;
  render_pass_create_info.subpassCount = 1;
  render_pass_create_info.pSubpasses = &subpass;
  render_pass_create_info.dependencyCount = 1;
  render_pass_create_info.pDependencies = &dependency;
  if (vkCreateRenderPass(device_, &render_pass_create_info, nullptr, &render_pass_) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create render pass");
  }
}


void VulkanManager::createGraphicsPipeline()
{
  auto main_shaderv_code = readFile(std::string(xstr(SHADER_DIRECTORY)) + "/mainv.spv");
  auto main_shaderf_code = readFile(std::string(xstr(SHADER_DIRECTORY)) + "/mainf.spv");

  VkShaderModule main_shaderv_module = createShaderModule(main_shaderv_code);
  VkShaderModule main_shaderf_module = createShaderModule(main_shaderf_code);

  VkPipelineShaderStageCreateInfo main_shaderv_stage_create_info{};
  main_shaderv_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  main_shaderv_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
  main_shaderv_stage_create_info.module = main_shaderv_module;
  main_shaderv_stage_create_info.pName = "main"; // entrypoint : begin at function "main" within shader

  VkPipelineShaderStageCreateInfo main_shaderf_stage_create_info{};
  main_shaderf_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  main_shaderf_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  main_shaderf_stage_create_info.module = main_shaderf_module;
  main_shaderf_stage_create_info.pName = "main";


  VkPipelineShaderStageCreateInfo shader_stages[] = {main_shaderv_stage_create_info, main_shaderf_stage_create_info};

  VkPipelineVertexInputStateCreateInfo vertex_input_create_info{};
  vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_create_info.vertexBindingDescriptionCount = 0;
  vertex_input_create_info.pVertexBindingDescriptions = nullptr;
  vertex_input_create_info.vertexAttributeDescriptionCount = 0;
  vertex_input_create_info.pVertexAttributeDescriptions = nullptr;

  VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info{};
  input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

  /*
  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)swap_chain_extent_.width;
  viewport.height = (float)swap_chain_extent_.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swap_chain_extent_;
  */
  VkPipelineViewportStateCreateInfo viewport_state_create_info{};
  viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state_create_info.viewportCount = 1;
  viewport_state_create_info.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterization_state_create_info{};
  rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterization_state_create_info.depthClampEnable = VK_FALSE;
  rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
  rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
  rasterization_state_create_info.lineWidth = 1.0f;
  rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterization_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterization_state_create_info.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling_create_info{};
  multisampling_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling_create_info.sampleShadingEnable = VK_FALSE;
  multisampling_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState color_blend_attachment{};
  color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attachment.blendEnable = VK_FALSE;
  /*
  color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
  color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
  color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
  color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
  color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
  color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
  */

  VkPipelineColorBlendStateCreateInfo color_blending_create_info{};
  color_blending_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blending_create_info.logicOpEnable = VK_FALSE;
  color_blending_create_info.logicOp = VK_LOGIC_OP_COPY; // Optional
  color_blending_create_info.attachmentCount = 1;
  color_blending_create_info.pAttachments = &color_blend_attachment;

  std::vector<VkDynamicState> dynamic_states = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };
  VkPipelineDynamicStateCreateInfo dynamic_state_create_info{};
  dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state_create_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
  dynamic_state_create_info.pDynamicStates = dynamic_states.data();

  VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
  pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_create_info.setLayoutCount = 0;
  pipeline_layout_create_info.pushConstantRangeCount = 0;
  if (vkCreatePipelineLayout(device_, &pipeline_layout_create_info, nullptr, &pipeline_layout_) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create pipeline layout");
  }


  VkGraphicsPipelineCreateInfo pipeline_create_info{};
  pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_create_info.stageCount = 2;
  pipeline_create_info.pStages = shader_stages;
  pipeline_create_info.pVertexInputState = &vertex_input_create_info;
  pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
  pipeline_create_info.pViewportState = &viewport_state_create_info;
  pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
  pipeline_create_info.pMultisampleState = &multisampling_create_info;
  pipeline_create_info.pDepthStencilState = nullptr;
  pipeline_create_info.pColorBlendState = &color_blending_create_info;
  pipeline_create_info.pDynamicState = &dynamic_state_create_info;
  pipeline_create_info.layout = pipeline_layout_;
  pipeline_create_info.renderPass = render_pass_;
  pipeline_create_info.subpass = 0;
  pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
  pipeline_create_info.basePipelineIndex = -1;
  if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &graphics_pipeline_) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create graphics pipeline");
  }


  vkDestroyShaderModule(device_, main_shaderv_module, nullptr);
  vkDestroyShaderModule(device_, main_shaderf_module, nullptr);
  return;
}


void VulkanManager::createFramebuffers()
{
  swap_chain_framebuffers_.resize(swap_chain_image_views_.size());
  for (size_t i = 0; i < swap_chain_image_views_.size(); i++)
  {
    VkImageView attachments[] = { swap_chain_image_views_[i] };
    VkFramebufferCreateInfo framebuffer_info{};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = render_pass_;
    framebuffer_info.attachmentCount = 1;
    framebuffer_info.pAttachments = attachments;
    framebuffer_info.width = swap_chain_extent_.width;
    framebuffer_info.height = swap_chain_extent_.height;
    framebuffer_info.layers = 1;
    if (vkCreateFramebuffer(device_, &framebuffer_info, nullptr, &swap_chain_framebuffers_[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create framebuffer");
    }
  }
  return;
}


void VulkanManager::createCommandPool()
{
  QueueFamilyIndices queue_family_indices = findQueueFamilies(physical_device_);
  VkCommandPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();
  if (vkCreateCommandPool(device_, &pool_info, nullptr, &command_pool_) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create command pool");
  }
}


void VulkanManager::createCommandBuffer()
{
  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = command_pool_;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = 1;
  if (vkAllocateCommandBuffers(device_, &alloc_info, &command_buffer_) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate command buffers");
  }
}


void VulkanManager::createSyncObjects()
{
  VkSemaphoreCreateInfo semaphore_info{};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VkFenceCreateInfo fence_info{};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Begin with the fence activated to avoid hang on startup

  if (vkCreateSemaphore(device_, &semaphore_info, nullptr, &image_available_semaphore_) != VK_SUCCESS ||
      vkCreateSemaphore(device_, &semaphore_info, nullptr, &render_finished_semaphore_) != VK_SUCCESS ||
      vkCreateFence(device_, &fence_info, nullptr, &in_flight_fence_) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create sync objects");
  }

}


VkSurfaceFormatKHR VulkanManager::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats)
{
  for (const auto& available_format : available_formats) {
    if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return available_format;
    }
  }
  return available_formats[0];
}

VkPresentModeKHR VulkanManager::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes) {
    for (const auto& available_present_mode : available_present_modes) {
        if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return available_present_mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanManager::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
  {
    return capabilities.currentExtent;
  }

  int width, height;
  glfwGetFramebufferSize(window_, &width, &height);

  VkExtent2D actualExtent = {
    static_cast<uint32_t>(width),
    static_cast<uint32_t>(height)
  };

  actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
  actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

  return actualExtent;
}


std::vector<const char*> VulkanManager::getRequiredInstanceExtensions()
{
  uint32_t glfw_extension_count = 0;
  const char** glfw_extensions;
  glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

  std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

  if (enable_validation_layers_)
  {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}


void VulkanManager::recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index)
{
  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  if (vkBeginCommandBuffer(command_buffer_, &begin_info) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to begin recording command buffer");
  }

  VkRenderPassBeginInfo render_pass_info{};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.renderPass = render_pass_;
  render_pass_info.framebuffer = swap_chain_framebuffers_[image_index];
  render_pass_info.renderArea.offset = {0, 0};
  render_pass_info.renderArea.extent = swap_chain_extent_;
  VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  render_pass_info.clearValueCount = 1;
  render_pass_info.pClearValues = &clear_color;
  vkCmdBeginRenderPass(command_buffer_, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_);
  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(swap_chain_extent_.width);
  viewport.height = static_cast<float>(swap_chain_extent_.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(command_buffer_, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swap_chain_extent_;
  vkCmdSetScissor(command_buffer_, 0, 1, &scissor);

  vkCmdDraw(command_buffer_, 3, 1, 0, 0);
  vkCmdEndRenderPass(command_buffer_);
  if (vkEndCommandBuffer(command_buffer_) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to record command buffer");
  }
}


VulkanManager::QueueFamilyIndices VulkanManager::findQueueFamilies(VkPhysicalDevice device)
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
      indices.graphics_family = i;
    }
    VkBool32 present_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &present_support);
    if (present_support)
    {
      indices.present_family = i;
    }
    if (indices.isComplete())
      break;
    i++;
  }
  return indices;
}

VulkanManager::SwapChainSupportDetails VulkanManager::querySwapChainSupport(VkPhysicalDevice device)
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


std::vector<char> VulkanManager::readFile(const std::string &filename)
{
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file.is_open())
  {
    throw std::runtime_error("Failed to open file: " + filename);
  }
  size_t file_size = (size_t)file.tellg();
  std::vector<char> buffer(file_size);

  file.seekg(0);
  file.read(buffer.data(), file_size);
  file.close();

  return buffer;
}

VkShaderModule VulkanManager::createShaderModule(const std::vector<char> &code)
{
  VkShaderModuleCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = code.size();
  create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

  VkShaderModule shader_module;
  if (vkCreateShaderModule(device_, &create_info, nullptr, &shader_module) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create shader module");
  }
  return shader_module;
}


bool VulkanManager::checkValidationLayerSupport()
{
  uint32_t layer_count;
  vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
  std::vector<VkLayerProperties> available_layers(layer_count);
  vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

  for (const char* layer_name : validation_layers_)
  {
    bool layer_found = false;
    for (const auto& layer_properties : available_layers)
    {
      if (!strcmp(layer_name, layer_properties.layerName))
      {
        layer_found = true;
        break;
      }
    }
    if (!layer_found)
    {
      return false;
    }
  }
  return true;
}


} // namespace Anthrax
