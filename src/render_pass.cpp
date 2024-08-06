/* ---------------------------------------------------------------- *\
 * render_pass.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-07-20
\* ---------------------------------------------------------------- */

#include "render_pass.hpp"

namespace Anthrax
{

RenderPass::RenderPass(Device device, std::string shadercode_file_prefix)
{
	device_ = device;
	shadercode_file_prefix_ = shadercode_file_prefix;

	return;
}


void RenderPass::init()
{
	VkAttachmentDescription color_attachment{};
	color_attachment.format = swap_chain_.format();
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
	if (vkCreateRenderPass(device_.logical, &render_pass_create_info, nullptr, &render_pass_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create render pass");
	}

	/*
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

	if (vkCreateDescriptorSetLayout(device_.logical, &layout_info, nullptr, &descriptor_set_layout_) != VK_SUCCESS)
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
	
	return;
	*/

	// Create graphics pipeline
	pipeline_ = GraphicsPipeline(device_, shadercode_file_prefix_);
	pipeline_.linkToRenderPass(render_pass_, 0);
	pipeline_.create();
}


void RenderPass::addBuffer(Buffer buffer)
{
	buffers_.push_back(buffer);
	return;
}


RenderPass::~RenderPass()
{
	//destroy();

	return;
}


void RenderPass::destroy()
{
	pipeline_.destroy();
	vkDestroyRenderPass(device_.logical, render_pass_, nullptr);
	/*
	for (unsigned int i = 0; i < buffers_.size(); i++)
	{
		buffers_[i].destroy();
	}
	*/

	return;
}


void RenderPass::recordCommandBuffer(VkCommandBuffer command_buffer, VkFramebuffer framebuffer)
{
	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to begin recording command buffer");
	}

	VkRenderPassBeginInfo render_pass_info{};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.renderPass = render_pass_;
	render_pass_info.framebuffer = framebuffer;
	render_pass_info.renderArea.offset = {0, 0};
	render_pass_info.renderArea.extent = swap_chain_.extent();
	VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
	render_pass_info.clearValueCount = 1;
	render_pass_info.pClearValues = &clear_color;
	vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.data());
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swap_chain_.extent().width);
	viewport.height = static_cast<float>(swap_chain_.extent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(command_buffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = swap_chain_.extent();
	vkCmdSetScissor(command_buffer, 0, 1, &scissor);

	vkCmdDraw(command_buffer, 3, 1, 0, 0);
	vkCmdEndRenderPass(command_buffer);
	if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to record command buffer");
	}
	return;
}

} // namespace Anthrax
