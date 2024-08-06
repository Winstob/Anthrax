/* ---------------------------------------------------------------- *\
 * descriptor.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-08-06
\* ---------------------------------------------------------------- */

#include "descriptor.hpp"

namespace Anthrax
{

Descriptor::Descriptor(Device device, ShaderStage stage, std::vector<Buffer> buffers, std::vector<Image> images)
{
	device_ = device;

	VkShaderStageFlagBits shader_stage;
	switch (stage)
	{
		case ShaderStage::VERTEX:
			shader_stage = VK_SHADER_STAGE_VERTEX_BIT;
			break;
		case ShaderStage::GEOMETRY:
			shader_stage = VK_SHADER_STAGE_GEOMETRY_BIT;
			break;
		case ShaderStage::FRAGMENT:
			shader_stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			break;
		case ShaderStage::COMPUTE:
			shader_stage = VK_SHADER_STAGE_COMPUTE_BIT;
			break;
	}

	// Create descriptor set layout
	VkDescriptorSetLayoutBinding layout_bindings[buffers.size() + images.size()];
	unsigned int i;
	for (i = 0; i < buffers.size(); i++)
	{
		VkDescriptorType descriptor_type;
		switch (buffers[i].type())
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
		layout_bindings[i].stageFlags = shader_stage;
	}
	for (unsigned int j = 0; j < images.size(); j++)
	{
		i++;
		layout_bindings[i].binding = i;
		layout_bindings[i].descriptorCount = 1;
		layout_bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		layout_bindings[i].pImmutableSamplers = nullptr;
		layout_bindings[i].stageFlags = shader_stage;
	}
	VkDescriptorSetLayoutCreateInfo layout_info{};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount = buffers_.size();
	layout_info.pBindings = layout_bindings;

	if (vkCreateDescriptorSetLayout(device_.logical, &layout_info, nullptr, &descriptor_set_layout_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create compute descriptor set layout!");
	}

	// Create descriptor pool
	std::array<VkDescriptorPoolSize, 3> pool_sizes{};
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_sizes[0].descriptorCount = 1;
	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	pool_sizes[1].descriptorCount = 1;
	pool_sizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	pool_sizes[2].descriptorCount = 1;
	VkDescriptorPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.poolSizeCount = pool_sizes.size();
	pool_info.pPoolSizes = pool_sizes.data();
	pool_info.maxSets = buffers.size() + images.size();
	if (vkCreateDescriptorPool(device_.logical, &pool_info, nullptr, &descriptor_pool_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create compute descriptor pool!");
	}

	// Create descriptor sets
	VkDescriptorSetAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = descriptor_pool_;
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &descriptor_set_layout_;
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

	/*
	// Create shader module
	Shader shader(device_, shadercode_filename_);

	// Create pipeline
	VkPipelineLayoutCreateInfo pipeline_layout_info{};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &descriptor_set_layout_;
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
	*/
	
	return;
}


Descriptor::~Descriptor()
{
	destroy();

	return;
}


void Descriptor::destroy()
{
	if (descriptor_pool_ != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(device_.logical, descriptor_pool_, nullptr);
	if (descriptor_set_layout_ != VK_NULL_HANDLE)
		vkDestroyDescriptorSetLayout(device_.logical, descriptor_set_layout_, nullptr);
	/*
	for (unsigned int i = 0; i < buffers_.size(); i++)
	{
		buffers_[i].destroy();
	}
	*/

	return;
}


} // namespace Anthrax