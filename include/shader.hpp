/* ---------------------------------------------------------------- *\
 * shader.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-02-03
\* ---------------------------------------------------------------- */


#ifndef SHADER_HPP
#define SHADER_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <vector>


namespace Anthrax
{

class Shader
{
public:
  Shader(VkDevice device, std::string input_file);
  ~Shader();
  VkShaderModule data() { return module_; }

private:
  std::vector<char> readFile(const std::string &filename);
  VkShaderModule createShaderModule(const std::vector<char> &code);

  VkDevice device_;
  VkShaderModule module_;

};

} // namespace Anthrax

#endif // SHADER_HPP
