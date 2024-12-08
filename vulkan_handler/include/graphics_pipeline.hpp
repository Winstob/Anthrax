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
#include "descriptor.hpp"


namespace Anthrax
{

class GraphicsPipeline
{
public:
	GraphicsPipeline() {};
	GraphicsPipeline(Device device, std::string shadercode_file_prefix);
	~GraphicsPipeline();
	VkPipeline data() { return pipeline_; }
	VkPipelineLayout getLayout() { return pipeline_layout_; }
	//VkDescriptorSet *getDescriptorSetPtr() { return descriptor_.getDescriptorSetPtr(); }
	VkDescriptorSet *getDescriptorSetPtr() { return fragment_descriptors_[fragment_descriptor_index_].getDescriptorSetPtr(); }

	void destroy();

	void setFragmentDescriptors(std::vector<Descriptor> descriptors) { fragment_descriptors_ = descriptors; }
	void selectFragmentDescriptor(int index) { fragment_descriptor_index_ = index; }

	void linkToRenderPass(VkRenderPass render_pass, int subpass_index);
	void init();

private:

	Device device_;

	std::string shadercode_file_prefix_;

	std::vector<Descriptor> fragment_descriptors_;
	int fragment_descriptor_index_;
	VkPipelineLayout pipeline_layout_;
	VkPipeline pipeline_;
	VkRenderPass render_pass_;
	int subpass_index_;

};

} // namespace Anthrax

#endif // GRAPHICS_PIPELINE_HPP
