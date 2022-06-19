#include "GBufferRenderPass.h"
#include <array>

#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"

#include "Window.h"
#include "VulkanRenderer.h"
#include "VulkanUtils.h"
#include "VulkanFramebufferAttachment.h"

#include "../shaders/shared_structs.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

DECLARE_RENDERPASS(GBufferRenderPass);

#include "DeferredCompositionRenderpass.h"
DECLARE_RENDERPASS(DeferredCompositionRenderpass);

void GBufferRenderPass::Init()
{
	SetupRenderpass();
	SetupFramebuffer();
	CreateSampler();
	CreateDescriptors();
	CreatePipeline();
}

void GBufferRenderPass::Draw()
{
	auto& m_device = VulkanRenderer::m_device;
	auto& m_swapchain = VulkanRenderer::m_swapchain;
	auto& commandBuffers = VulkanRenderer::commandBuffers;
	auto& swapchainIdx = VulkanRenderer::swapchainIdx;
	auto* windowPtr = VulkanRenderer::windowPtr;

	// Clear values for all attachments written in the fragment shader
	std::array<VkClearValue,4> clearValues;
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[2].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	clearValues[3].depthStencil = { 1.0f, 0 };
	
	VkRenderPassBeginInfo renderPassBeginInfo = oGFX::vk::inits::renderPassBeginInfo();
	renderPassBeginInfo.renderPass =  deferredPass;
	renderPassBeginInfo.framebuffer = deferredFB;
	renderPassBeginInfo.renderArea.extent.width = m_swapchain.swapChainExtent.width;
	renderPassBeginInfo.renderArea.extent.height = m_swapchain.swapChainExtent.height;
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassBeginInfo.pClearValues = clearValues.data();
	
	vkCmdBeginRenderPass(commandBuffers[swapchainIdx], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	
	VkViewport viewport = { 0, float(windowPtr->m_height), float(windowPtr->m_width), -float(windowPtr->m_height), 0, 1 };
	VkRect2D scissor = { {0, 0}, {uint32_t(windowPtr->m_width),uint32_t(windowPtr->m_height) } };
	vkCmdSetViewport(commandBuffers[swapchainIdx], 0, 1, &viewport);
	vkCmdSetScissor(commandBuffers[swapchainIdx], 0, 1, &scissor);
	
	vkCmdBindPipeline(commandBuffers[swapchainIdx], VK_PIPELINE_BIND_POINT_GRAPHICS, deferredPipe);
	std::array<VkDescriptorSet, 3> descriptorSetGroup = {VulkanRenderer::g0_descriptors, VulkanRenderer::uniformDescriptorSets[swapchainIdx],
		VulkanRenderer::globalSamplers };
	
	uint32_t dynamicOffset = 0;
	vkCmdBindDescriptorSets(commandBuffers[swapchainIdx],VK_PIPELINE_BIND_POINT_GRAPHICS,VulkanRenderer:: indirectPipeLayout,
		0, static_cast<uint32_t>(descriptorSetGroup.size()), descriptorSetGroup.data(), 1, &dynamicOffset);
	
	for (auto& entity : VulkanRenderer::entities)
	{
		auto& model = VulkanRenderer::models[entity.modelID];
	
		glm::mat4 xform(1.0f);
		xform = glm::translate(xform, entity.pos);
		xform = glm::rotate(xform,glm::radians(entity.rot), entity.rotVec);
		xform = glm::scale(xform, entity.scale);
	
		vkCmdPushConstants(commandBuffers[swapchainIdx],
			VulkanRenderer::indirectPipeLayout,
			VK_SHADER_STAGE_VERTEX_BIT,	// stage to push constants to
			0,							// offset of push constants to update
			sizeof(glm::mat4),			// size of data being pushed
			glm::value_ptr(xform));		// actualy data being pushed (could be an array));
	
		VkDeviceSize offsets[] = { 0 };	
		vkCmdBindIndexBuffer(commandBuffers[swapchainIdx], VulkanRenderer::g_indexBuffer.getBuffer(),0, VK_INDEX_TYPE_UINT32);
		vkCmdBindVertexBuffers(commandBuffers[swapchainIdx], VERTEX_BUFFER_ID, 1, VulkanRenderer::g_vertexBuffer.getBufferPtr(), offsets);
	
		// bind instance buffer
		vkCmdBindVertexBuffers(commandBuffers[swapchainIdx], INSTANCE_BUFFER_ID, 1, &VulkanRenderer::instanceBuffer.buffer, offsets);
		//vkCmdDrawIndexed(commandBuffers[swapchainImageIndex], model.indices.count, 1, model.indices.offset, model.vertices.offset, 0);
	}
	
	//if (m_device.enabledFeatures.multiDrawIndirect)
	//{
	//	vkCmdDrawIndexedIndirect(commandBuffers[swapchainImageIndex], indirectCommandsBuffer.buffer, 0, indirectDrawCount, sizeof(VkDrawIndexedIndirectCommand));
	//}
	//else
	{
		for (size_t i = 0; i < VulkanRenderer::indirectCommands.size(); i++)
		{		
			vkCmdDrawIndexedIndirect(commandBuffers[swapchainIdx], VulkanRenderer::indirectCommandsBuffer.buffer, i * sizeof(VkDrawIndexedIndirectCommand), 1, sizeof(VkDrawIndexedIndirectCommand));
		}
	}
	// End Render  Pass
	vkCmdEndRenderPass(commandBuffers[swapchainIdx]);
}

void GBufferRenderPass::Shutdown()
{
	auto& m_device = VulkanRenderer::m_device;
	att_albedo.destroy(m_device.logicalDevice);
	att_position.destroy(m_device.logicalDevice);
	att_normal.destroy(m_device.logicalDevice);
	att_depth.destroy(m_device.logicalDevice);
	vkDestroyFramebuffer(m_device.logicalDevice, deferredFB, nullptr);
	vkDestroySampler(m_device.logicalDevice, deferredSampler, nullptr);
	vkDestroyRenderPass(m_device.logicalDevice,deferredPass, nullptr);
	vkDestroyPipeline(m_device.logicalDevice, deferredPipe, nullptr);
}

void GBufferRenderPass::SetupRenderpass()
{
	auto& m_device = VulkanRenderer::m_device;
	auto& m_swapchain = VulkanRenderer::m_swapchain;

	att_position.createAttachment(m_device, m_swapchain.swapChainExtent.width,  m_swapchain.swapChainExtent.height,
		VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

	att_normal.createAttachment(m_device,m_swapchain.swapChainExtent.width,  m_swapchain.swapChainExtent.height,
		VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

	att_albedo.createAttachment(m_device, m_swapchain.swapChainExtent.width,  m_swapchain.swapChainExtent.height,
		VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

	VkFormat depF = oGFX::ChooseSupportedFormat(m_device,
		{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	att_depth.createAttachment(m_device, m_swapchain.swapChainExtent.width,  m_swapchain.swapChainExtent.height,
		depF, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

	// Set up separate renderpass with references to the color and depth attachments
	std::array<VkAttachmentDescription, 4> attachmentDescs = {};

	// Init attachment properties
	for (uint32_t i = 0; i < 4; ++i)
	{
		attachmentDescs[i].samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescs[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescs[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescs[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescs[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		if (i == 3)
		{
			// depth format
			attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}
		else
		{
			attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
	}

	// Formats
	attachmentDescs[0].format = att_position.format;
	attachmentDescs[1].format = att_normal.format;
	attachmentDescs[2].format = att_albedo.format;
	attachmentDescs[3].format = att_depth.format;

	std::vector<VkAttachmentReference> colorReferences;
	colorReferences.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
	colorReferences.push_back({ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
	colorReferences.push_back({ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 3;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.pColorAttachments = colorReferences.data();
	subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
	subpass.pDepthStencilAttachment = &depthReference;

	// Use subpass dependencies for attachment layout transitions
	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pAttachments = attachmentDescs.data();
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies = dependencies.data();

	VK_CHK(vkCreateRenderPass(m_device.logicalDevice, &renderPassInfo, nullptr, &deferredPass));
}

void GBufferRenderPass::SetupFramebuffer()
{
	std::array<VkImageView,4> attachments;
	attachments[0] = att_position.view;
	attachments[1] = att_normal.view;
	attachments[2] = att_albedo.view;
	attachments[3] = VulkanRenderer::m_swapchain.depthAttachment.view;

	VkFramebufferCreateInfo fbufCreateInfo = {};
	fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbufCreateInfo.pNext = NULL;
	fbufCreateInfo.renderPass = deferredPass;
	fbufCreateInfo.pAttachments = attachments.data();
	fbufCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	fbufCreateInfo.width = VulkanRenderer::m_swapchain.swapChainExtent.width;
	fbufCreateInfo.height = VulkanRenderer::m_swapchain.swapChainExtent.height;
	fbufCreateInfo.layers = 1;
	VK_CHK(vkCreateFramebuffer(VulkanRenderer::m_device.logicalDevice, &fbufCreateInfo, nullptr, &deferredFB));

	deferredImg[0] = ImGui_ImplVulkan_AddTexture(deferredSampler, att_position.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	deferredImg[1] = ImGui_ImplVulkan_AddTexture(deferredSampler, att_normal.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	deferredImg[2] = ImGui_ImplVulkan_AddTexture(deferredSampler, att_albedo.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	deferredImg[3] = ImGui_ImplVulkan_AddTexture(deferredSampler, att_depth.view, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

}

void GBufferRenderPass::CreateSampler()
{
	// Create sampler to sample from the color attachments
	VkSamplerCreateInfo sampler = oGFX::vk::inits::samplerCreateInfo();
	sampler.magFilter = VK_FILTER_NEAREST;
	sampler.minFilter = VK_FILTER_NEAREST;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeV = sampler.addressModeU;
	sampler.addressModeW = sampler.addressModeU;
	sampler.mipLodBias = 0.0f;
	sampler.maxAnisotropy = 1.0f;
	sampler.minLod = 0.0f;
	sampler.maxLod = 1.0f;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	VK_CHK(vkCreateSampler(VulkanRenderer::m_device.logicalDevice, &sampler, nullptr, &deferredSampler));
}

void GBufferRenderPass::CreateDescriptors()
{
	auto& m_device = VulkanRenderer::m_device;

	if (VulkanRenderer::deferredSet) return;

	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(VulkanRenderer::m_device.physicalDevice,&props);
	size_t minUboAlignment = props.limits.minUniformBufferOffsetAlignment;
	if (minUboAlignment > 0) {
		uboDynamicAlignment = (uboDynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
	}

	size_t numLights = 1;
	VkDeviceSize vpBufferSize = uboDynamicAlignment * numLights;

	//// LightData bufffer size

	// Image descriptors for the offscreen color attachments
	VkDescriptorImageInfo texDescriptorPosition =
		oGFX::vk::inits::descriptorImageInfo(
			deferredSampler,
			att_position.view,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VkDescriptorImageInfo texDescriptorNormal =
		oGFX::vk::inits::descriptorImageInfo(
			deferredSampler,
			att_normal.view,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VkDescriptorImageInfo texDescriptorAlbedo =
		oGFX::vk::inits::descriptorImageInfo(
			deferredSampler,
			att_albedo.view, 
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	DescriptorBuilder::Begin(&VulkanRenderer::DescLayoutCache, &VulkanRenderer::DescAlloc)
		.BindImage(1, &texDescriptorPosition, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.BindImage(2, &texDescriptorNormal, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.BindImage(3, &texDescriptorAlbedo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.BindBuffer(4, &VulkanRenderer::lightsBuffer.descriptor, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.Build(VulkanRenderer::deferredSet,VulkanRenderer::deferredSetLayout);
}

void GBufferRenderPass::CreatePipeline()
{
	auto& m_device = VulkanRenderer::m_device;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = oGFX::vk::inits::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	VkPipelineRasterizationStateCreateInfo rasterizationState = oGFX::vk::inits::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	VkPipelineColorBlendAttachmentState blendAttachmentState = oGFX::vk::inits::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
	VkPipelineColorBlendStateCreateInfo colorBlendState = oGFX::vk::inits::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
	VkPipelineDepthStencilStateCreateInfo depthStencilState = oGFX::vk::inits::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
	VkPipelineViewportStateCreateInfo viewportState = oGFX::vk::inits::pipelineViewportStateCreateInfo(1, 1, 0);
	VkPipelineMultisampleStateCreateInfo multisampleState = oGFX::vk::inits::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
	std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamicState = oGFX::vk::inits::pipelineDynamicStateCreateInfo(dynamicStateEnables);
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

	VkGraphicsPipelineCreateInfo pipelineCI = oGFX::vk::inits::pipelineCreateInfo(VulkanRenderer::indirectPipeLayout, VulkanRenderer::defaultRenderPass);
	pipelineCI.pInputAssemblyState = &inputAssemblyState;
	pipelineCI.pRasterizationState = &rasterizationState;
	pipelineCI.pColorBlendState = &colorBlendState;
	pipelineCI.pMultisampleState = &multisampleState;
	pipelineCI.pViewportState = &viewportState;
	pipelineCI.pDepthStencilState = &depthStencilState;
	pipelineCI.pDynamicState = &dynamicState;
	pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineCI.pStages = shaderStages.data();


	// Vertex input state from glTF model for pipeline rendering models
	//how the data for a single vertex (including infos such as pos, colour, texture, coords, normals etc) is as a whole
	std::vector<VkVertexInputBindingDescription> bindingDescription {	
		oGFX::vk::inits::vertexInputBindingDescription(VERTEX_BUFFER_ID,sizeof(oGFX::Vertex),VK_VERTEX_INPUT_RATE_VERTEX),

		oGFX::vk::inits::vertexInputBindingDescription(INSTANCE_BUFFER_ID,sizeof(oGFX::InstanceData),VK_VERTEX_INPUT_RATE_INSTANCE),
	};

	//how the data for an attirbute is define in the vertex
	std::vector<VkVertexInputAttributeDescription>attributeDescriptions{
		oGFX::vk::inits::vertexInputAttributeDescription(VERTEX_BUFFER_ID,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(oGFX::Vertex, pos)), //Position attribute
		oGFX::vk::inits::vertexInputAttributeDescription(VERTEX_BUFFER_ID,1,VK_FORMAT_R32G32B32_SFLOAT,offsetof(oGFX::Vertex, norm)),//normals attribute
		oGFX::vk::inits::vertexInputAttributeDescription(VERTEX_BUFFER_ID,2,VK_FORMAT_R32G32B32_SFLOAT,offsetof(oGFX::Vertex, col)),//col attribute
		oGFX::vk::inits::vertexInputAttributeDescription(VERTEX_BUFFER_ID,3,VK_FORMAT_R32G32B32_SFLOAT,offsetof(oGFX::Vertex, tangent)),//tangent attribute
		oGFX::vk::inits::vertexInputAttributeDescription(VERTEX_BUFFER_ID,4,VK_FORMAT_R32G32_SFLOAT	  ,offsetof(oGFX::Vertex, tex)),    //Texture attribute

		oGFX::vk::inits::vertexInputAttributeDescription(INSTANCE_BUFFER_ID,15,VK_FORMAT_R32G32B32A32_UINT,offsetof(oGFX::InstanceData,instanceAttributes)),    // instance attribute
	};

	rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
	// -- VERTEX INPUT -- 
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = oGFX::vk::inits::pipelineVertexInputStateCreateInfo(bindingDescription,attributeDescriptions);
	//vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
	//vertexInputCreateInfo.vertexAttributeDescriptionCount = 5;

	vertexInputCreateInfo.vertexBindingDescriptionCount = bindingDescription.size()	;
	vertexInputCreateInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();

	pipelineCI.pVertexInputState = &vertexInputCreateInfo;

	// Offscreen pipeline
	shaderStages[0]  = VulkanRenderer::LoadShader(m_device, "Shaders/mrt.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = VulkanRenderer::LoadShader(m_device, "Shaders/mrt.frag.spv",VK_SHADER_STAGE_FRAGMENT_BIT);

	// Separate render pass
	pipelineCI.renderPass = deferredPass;

	// Blend attachment states required for all color attachments
	// This is important, as color write mask will otherwise be 0x0 and you
	// won't see anything rendered to the attachment
	std::array<VkPipelineColorBlendAttachmentState, 3> blendAttachmentStates = {
		oGFX::vk::inits::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
		oGFX::vk::inits::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
		oGFX::vk::inits::pipelineColorBlendAttachmentState(0xf, VK_FALSE)
	};

	colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
	colorBlendState.pAttachments = blendAttachmentStates.data();

	VK_CHK(vkCreateGraphicsPipelines(m_device.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &deferredPipe));
	vkDestroyShaderModule(m_device.logicalDevice, shaderStages[0].module, nullptr);
	vkDestroyShaderModule(m_device.logicalDevice, shaderStages[1].module, nullptr);

}
