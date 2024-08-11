/* ---------------------------------------------------------------- *\
 * graphics_pipeline.cpp
 * Author: Gavin Ralston
 * Date Created: 2024-05-04
\* ---------------------------------------------------------------- */

#include <iostream>

#include "graphics_pipeline.hpp"
#include "tools.hpp"


namespace Anthrax
{


GraphicsPipeline::GraphicsPipeline(Device device, std::string shadercode_file_prefix)
{
	device_ = device;
	shadercode_file_prefix_ = shadercode_file_prefix;
	return;
}


GraphicsPipeline::~GraphicsPipeline()
{
	// destroy();
	return;
}


void GraphicsPipeline::destroy()
{
	vkDestroyPipeline(device_.logical, pipeline_, nullptr);
	vkDestroyPipelineLayout(device_.logical, pipeline_layout_, nullptr);
	descriptor_.destroy();
	return;
}


void GraphicsPipeline::linkToRenderPass(VkRenderPass render_pass, int subpass_index)
{
	render_pass_ = render_pass;
	subpass_index_ = subpass_index;
	return;
}


void GraphicsPipeline::init()
{
	Shader main_shaderv = Shader(device_, std::string(xstr(SHADER_DIRECTORY)) + shadercode_file_prefix_ + "v.spv");
	Shader main_shaderf = Shader(device_, std::string(xstr(SHADER_DIRECTORY)) + shadercode_file_prefix_ + "f.spv");

	VkPipelineShaderStageCreateInfo main_shaderv_stage_create_info{};
	main_shaderv_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	main_shaderv_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	main_shaderv_stage_create_info.module = main_shaderv.data();
	main_shaderv_stage_create_info.pName = "main"; // entrypoint : begin at function "main" within shader

	VkPipelineShaderStageCreateInfo main_shaderf_stage_create_info{};
	main_shaderf_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	main_shaderf_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	main_shaderf_stage_create_info.module = main_shaderf.data();
	main_shaderf_stage_create_info.pName = "main";


	VkPipelineShaderStageCreateInfo shader_stages[] = {main_shaderv_stage_create_info, main_shaderf_stage_create_info};

	VkPipelineVertexInputStateCreateInfo vertex_input_create_info{};
	vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_create_info.vertexBindingDescriptionCount = 0;
	vertex_input_create_info.pVertexBindingDescriptions = nullptr;
	vertex_input_create_info.vertexAttributeDescriptionCount = 0;
	vertex_input_create_info.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info{};
	input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

	/*
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swap_chain_extent_.width;
	viewport.height = (float)swap_chain_extent_.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = swap_chain_extent_;
	*/
	VkPipelineViewportStateCreateInfo viewport_state_create_info{};
	viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_create_info.viewportCount = 1;
	viewport_state_create_info.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterization_state_create_info{};
	rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state_create_info.depthClampEnable = VK_FALSE;
	rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
	rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization_state_create_info.lineWidth = 1.0f;
	rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterization_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterization_state_create_info.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling_create_info{};
	multisampling_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling_create_info.sampleShadingEnable = VK_FALSE;
	multisampling_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState color_blend_attachment{};
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable = VK_FALSE;
	/*
	color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
	*/

	VkPipelineColorBlendStateCreateInfo color_blending_create_info{};
	color_blending_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending_create_info.logicOpEnable = VK_FALSE;
	color_blending_create_info.logicOp = VK_LOGIC_OP_COPY; // Optional
	color_blending_create_info.attachmentCount = 1;
	color_blending_create_info.pAttachments = &color_blend_attachment;

	std::vector<VkDynamicState> dynamic_states = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamic_state_create_info{};
	dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_create_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
	dynamic_state_create_info.pDynamicStates = dynamic_states.data();

	// Create descriptor set
	if (buffers_.size() + images_.size() > 0)
	{
		descriptor_ = Descriptor(device_, Descriptor::ShaderStage::FRAGMENT, buffers_, images_);
	}

	VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
	pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	if (buffers_.size() + images_.size() > 0)
	{
		pipeline_layout_create_info.setLayoutCount = buffers_.size();
		pipeline_layout_create_info.pSetLayouts = descriptor_.getDescriptorSetLayoutPtr();
	}
	else
	{
		pipeline_layout_create_info.setLayoutCount = 0;
	}
	//pipeline_layout_create_info.pushConstantRangeCount = 0;
	if (vkCreatePipelineLayout(device_.logical, &pipeline_layout_create_info, nullptr, &pipeline_layout_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create pipeline layout");
	}


	VkGraphicsPipelineCreateInfo pipeline_create_info{};
	pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_create_info.stageCount = 2;
	pipeline_create_info.pStages = shader_stages;
	pipeline_create_info.pVertexInputState = &vertex_input_create_info;
	pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
	pipeline_create_info.pViewportState = &viewport_state_create_info;
	pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
	pipeline_create_info.pMultisampleState = &multisampling_create_info;
	pipeline_create_info.pDepthStencilState = nullptr;
	pipeline_create_info.pColorBlendState = &color_blending_create_info;
	pipeline_create_info.pDynamicState = &dynamic_state_create_info;
	pipeline_create_info.layout = pipeline_layout_;
	pipeline_create_info.renderPass = render_pass_;
	pipeline_create_info.subpass = subpass_index_;
	pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	pipeline_create_info.basePipelineIndex = -1;
	if (vkCreateGraphicsPipelines(device_.logical, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &pipeline_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create graphics pipeline");
	}


	return;
}

} // namespace Anthrax
