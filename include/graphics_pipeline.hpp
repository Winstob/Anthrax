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
	GraphicsPipeline() : GraphicsPipeline(VK_NULL_HANDLE) {}
	GraphicsPipeline(VkDevice device);
	~GraphicsPipeline();
	VkPipeline data() { return pipeline_; }

	void linkToRenderPass(VkRenderPass render_pass, int subpass_index);
	void create();

private:

	VkDevice device_;
	VkPipeline pipeline_;

	VkPipelineLayout pipeline_layout_;
	VkRenderPass render_pass_;
	int subpass_index_;

};

} // namespace Anthrax

#endif // GRAPHICS_PIPELINE_HPP
