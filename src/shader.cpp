/* ---------------------------------------------------------------- *\
 * shader.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-05-04
\* ---------------------------------------------------------------- */

#include "shader.hpp"

#include <fstream>

namespace Anthrax
{


Shader::Shader(VkDevice device, std::string input_file)
{
  device_ = device;
  module_ = createShaderModule(readFile(input_file));
}


Shader::~Shader()
{
  vkDestroyShaderModule(device_, module_, nullptr);
  return;
}


std::vector<char> Shader::readFile(const std::string &filename)
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


VkShaderModule Shader::createShaderModule(const std::vector<char> &code)
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

} // namespace Anthrax
