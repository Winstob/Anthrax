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
	// Create descriptor set layout
	VkDescriptorSetLayoutBinding layout_bindings[buffers_.size()];
	for (unsigned int i = 0; i < buffers_.size(); i++)
	{
		VkDescriptorType descriptor_type;
		switch (buffers_[i].type())
		{
			case Buffer::UNIFORM_TYPE:
				descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				break;
			case Buffer::STORAGE_TYPE:
				descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				break;
		}
		layout_bindings[i].binding = i;
		layout_bindings[i].descriptorCount = 1;
		layout_bindings[i].descriptorType = descriptor_type;
		layout_bindings[i].pImmutableSamplers = nullptr;
		layout_bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	}
	VkDescriptorSetLayoutCreateInfo layout_info{};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount = buffers_.size();
	layout_info.pBindings = layout_bindings;

	VkDescriptorSetLayout descriptor_set_layout;
	if (vkCreateDescriptorSetLayout(device_.logical, &layout_info, nullptr, &descriptor_set_layout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create compute descriptor set layout!");
	}

	// Create descriptor pool
	std::array<VkDescriptorPoolSize, 2> pool_sizes{};
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_sizes[0].descriptorCount = 1;
	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	pool_sizes[1].descriptorCount = 1;
	VkDescriptorPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.poolSizeCount = pool_sizes.size();
	pool_info.pPoolSizes = pool_sizes.data();
	pool_info.maxSets = buffers_.size();
	if (vkCreateDescriptorPool(device_.logical, &pool_info, nullptr, &descriptor_pool_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create compute descriptor pool!");
	}

	// Create descriptor sets
	VkDescriptorSetAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = descriptor_pool_;
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &descriptor_set_layout;
	if (vkAllocateDescriptorSets(device_.logical, &alloc_info, &descriptor_set_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create compute descriptor set!");
	}
	VkWriteDescriptorSet descriptor_writes[buffers_.size()];
	for (unsigned int i = 0; i < buffers_.size(); i++)
	{
		VkDescriptorType descriptor_type;
		switch (buffers_[i].type())
		{
			case Buffer::UNIFORM_TYPE:
				descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				break;
			case Buffer::STORAGE_TYPE:
				descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				break;
		}
		VkDescriptorBufferInfo buffer_info{};
		buffer_info.buffer = buffers_[i].data();
		buffer_info.offset = 0;
		buffer_info.range = buffers_[i].size();

		descriptor_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[i].dstSet = descriptor_set_;
		descriptor_writes[i].dstBinding = i;
		descriptor_writes[i].dstArrayElement = 0;
		descriptor_writes[i].descriptorType = descriptor_type;
		descriptor_writes[i].descriptorCount = 1;
		descriptor_writes[i].pBufferInfo = &buffer_info;
	}
	vkUpdateDescriptorSets(device_.logical, buffers_.size(), descriptor_writes, 0, nullptr);

	// Create shader module
	Shader shader(device_.logical, shadercode_filename_);

	// Create pipeline
	VkPipelineLayoutCreateInfo pipeline_layout_info{};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
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

	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout_, 0, 1, &descriptor_set_, 0, nullptr);

	vkCmdDispatch(command_buffer, 1, 1, 1);

	if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to record compute command buffer!");
	}

	return;
}

} // namespace Anthrax
