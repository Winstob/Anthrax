/* ---------------------------------------------------------------- *\
 * compute_shader_manager.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-07-20
\* ---------------------------------------------------------------- */

#include "compute_shader_manager.hpp"

namespace Anthrax
{

ComputeShaderManager::ComputeShaderManager(Device device, std::string shadercode_filename)
{
	device_ = device;
	shadercode_filename_ = shadercode_filename;

	return;
}


void ComputeShaderManager::init()
{
	descriptor_ = Descriptor(device_, Descriptor::ShaderStage::COMPUTE, buffers_, std::vector<Image>());

	// Create shader module
	Shader shader(device_, shadercode_filename_);

	// Create pipeline
	VkPipelineLayoutCreateInfo pipeline_layout_info{};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = descriptor_.getDescriptorSetLayoutPtr();
	if (vkCreatePipelineLayout(device_.logical, &pipeline_layout_info, nullptr, &pipeline_layout_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create compute pipeline layout!");
	}
	VkPipelineShaderStageCreateInfo shader_stage_create_info{};
	shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage_create_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	shader_stage_create_info.module = shader.data();
	shader_stage_create_info.pName = "main";
	VkComputePipelineCreateInfo pipeline_info{};
	pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipeline_info.layout = pipeline_layout_;
	pipeline_info.stage = shader_stage_create_info;
	if (vkCreateComputePipelines(device_.logical, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create compute pipeline!");
	}
	
	return;
}


void ComputeShaderManager::addBuffer(Buffer buffer)
{
	buffers_.push_back(buffer);
	return;
}


ComputeShaderManager::~ComputeShaderManager()
{
	destroy();

	return;
}


void ComputeShaderManager::destroy()
{
	if (pipeline_ != VK_NULL_HANDLE)
		vkDestroyPipeline(device_.logical, pipeline_, nullptr);
	if (pipeline_layout_ != VK_NULL_HANDLE)
		vkDestroyPipelineLayout(device_.logical, pipeline_layout_, nullptr);
	descriptor_.destroy();
	/*
	for (unsigned int i = 0; i < buffers_.size(); i++)
	{
		buffers_[i].destroy();
	}
	*/

	return;
}


void ComputeShaderManager::recordCommandBuffer(VkCommandBuffer command_buffer)
{
	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to begin recording compute command buffer!");
	}

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);

	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout_, 0, 1, descriptor_.getDescriptorSetPtr(), 0, nullptr);

	vkCmdDispatch(command_buffer, 1, 1, 1);

	if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to record compute command buffer!");
	}

	return;
}

} // namespace Anthrax
