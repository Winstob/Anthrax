/* ---------------------------------------------------------------- *\
 * descriptor.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-08-06
\* ---------------------------------------------------------------- */

#include <iostream>

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

	unsigned int num_uniform_buffers = 0;
	unsigned int num_storage_buffers = 0;
	unsigned int num_storage_images = 0;

	// Create descriptor set layout
	std::vector<VkDescriptorSetLayoutBinding> layout_bindings(buffers.size() + images.size());
	unsigned int i;
	for (i = 0; i < buffers.size(); i++)
	{
		VkDescriptorType descriptor_type;
		switch (buffers[i].type())
		{
			case Buffer::UNIFORM_TYPE:
				descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				num_uniform_buffers++;
				break;
			case Buffer::STORAGE_TYPE:
				descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				num_storage_buffers++;
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
		num_storage_images++;
		layout_bindings[i].binding = i;
		layout_bindings[i].descriptorCount = 1;
		layout_bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		layout_bindings[i].pImmutableSamplers = nullptr;
		layout_bindings[i].stageFlags = shader_stage;
		i++;
	}
	VkDescriptorSetLayoutCreateInfo layout_info{};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount = buffers.size() + images.size();
	layout_info.pBindings = layout_bindings.data();

	if (vkCreateDescriptorSetLayout(device_.logical, &layout_info, nullptr, &descriptor_set_layout_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor set layout!");
	}

	// Create descriptor pool
	std::vector<VkDescriptorPoolSize> pool_sizes(0);
	if (num_uniform_buffers > 0)
	{
		pool_sizes.resize(pool_sizes.size()+1);
		pool_sizes[pool_sizes.size()-1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pool_sizes[pool_sizes.size()-1].descriptorCount = num_uniform_buffers;
	}
	if (num_storage_buffers > 0)
	{
		pool_sizes.resize(pool_sizes.size()+1);
		pool_sizes[pool_sizes.size()-1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		pool_sizes[pool_sizes.size()-1].descriptorCount = num_storage_buffers;
	}
	if (num_storage_images > 0)
	{
		pool_sizes.resize(pool_sizes.size()+1);
		pool_sizes[pool_sizes.size()-1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		pool_sizes[pool_sizes.size()-1].descriptorCount = num_storage_images;
	}

	VkDescriptorPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.poolSizeCount = pool_sizes.size();
	pool_info.pPoolSizes = pool_sizes.data();
	pool_info.maxSets = buffers.size() + images.size();
	if (vkCreateDescriptorPool(device_.logical, &pool_info, nullptr, &descriptor_pool_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool!");
	}

	// Create descriptor sets
	VkDescriptorSetAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = descriptor_pool_;
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &descriptor_set_layout_;
	if (vkAllocateDescriptorSets(device_.logical, &alloc_info, &descriptor_set_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor set!");
	}
	std::vector<VkWriteDescriptorSet> descriptor_writes(buffers.size() + images.size());
	std::vector<VkDescriptorBufferInfo> buffer_info(buffers.size());
	std::vector<VkDescriptorImageInfo> image_info(images.size());
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
		buffer_info[i].buffer = buffers[i].data();
		buffer_info[i].offset = 0;
		buffer_info[i].range = buffers[i].size();

		descriptor_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[i].dstSet = descriptor_set_;
		descriptor_writes[i].dstBinding = i;
		descriptor_writes[i].dstArrayElement = 0;
		descriptor_writes[i].descriptorType = descriptor_type;
		descriptor_writes[i].descriptorCount = 1;
		descriptor_writes[i].pBufferInfo = &(buffer_info[i]);
	}
	for (unsigned int j = 0; j < images.size(); j++)
	{
		image_info[j].sampler = VK_NULL_HANDLE;
		image_info[j].imageView = images[j].getImageView();
		image_info[j].imageLayout = images[j].getImageLayout();

		descriptor_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[i].dstSet = descriptor_set_;
		descriptor_writes[i].dstBinding = i;
		descriptor_writes[i].dstArrayElement = 0;
		descriptor_writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		descriptor_writes[i].descriptorCount = 1;
		descriptor_writes[i].pImageInfo = &(image_info[j]);
		i++;
	}

	vkUpdateDescriptorSets(device_.logical, buffers.size() + images.size(), descriptor_writes.data(), 0, nullptr);

	return;
}


Descriptor::~Descriptor()
{
	//destroy();

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
