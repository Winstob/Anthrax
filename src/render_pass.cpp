/* ---------------------------------------------------------------- *\
 * render_pass.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-07-20
\* ---------------------------------------------------------------- */

#include <iostream>

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

	// Create graphics pipeline
	pipeline_ = GraphicsPipeline(device_, shadercode_file_prefix_);
	pipeline_.linkToRenderPass(render_pass_, 0);
	for (unsigned int i = 0; i < buffers_.size(); i++)
	{
		pipeline_.addBuffer(buffers_[i]);
	}
	pipeline_.init();
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

	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.getLayout(), 0, 1, pipeline_.getDescriptorSetPtr(), 0, nullptr);

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
