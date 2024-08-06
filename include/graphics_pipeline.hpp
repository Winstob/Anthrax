/* ---------------------------------------------------------------- *\
 * graphics_pipeline.hpp
 * Author: Gavin Ralston
 * Date Created: 2024-05-04
\* ---------------------------------------------------------------- */


#ifndef GRAPHICS_PIPELINE_HPP
#define GRAPHICS_PIPELINE_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <vector>

#include "shader.hpp"


namespace Anthrax
{

class GraphicsPipeline
{
public:
	GraphicsPipeline() {};
	GraphicsPipeline(Device device, std::string shadercode_file_prefix);
	~GraphicsPipeline();
	VkPipeline data() { return pipeline_; }

	void destroy();

	void linkToRenderPass(VkRenderPass render_pass, int subpass_index);
	void create();

private:

	Device device_;
	VkPipeline pipeline_;

	std::string shadercode_file_prefix_;

	VkPipelineLayout pipeline_layout_;
	VkRenderPass render_pass_;
	int subpass_index_;

};

} // namespace Anthrax

#endif // GRAPHICS_PIPELINE_HPP