#include "ShadowPass.h"

#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"

#include "Window.h"
#include "VulkanRenderer.h"
#include "VulkanUtils.h"
#include "VulkanFramebufferAttachment.h"

#include "../shaders/shared_structs.h"
#include "MathCommon.h"

#include "DeferredCompositionRenderpass.h"

#include <array>

DECLARE_RENDERPASS(ShadowPass);

void ShadowPass::Init()
{
	SetupRenderpass();
	SetupFramebuffer();
}

void ShadowPass::CreatePSO()
{
	CreatePipeline();
}

void ShadowPass::Draw()
{
	if (!VulkanRenderer::deferredRendering)
		return;

	auto& device = VulkanRenderer::m_device;
	auto& swapchain = VulkanRenderer::m_swapchain;
	auto& commandBuffers = VulkanRenderer::commandBuffers;
	auto& swapchainIdx = VulkanRenderer::swapchainIdx;
	auto* windowPtr = VulkanRenderer::windowPtr;

    const VkCommandBuffer cmdlist = commandBuffers[swapchainIdx];
    PROFILE_GPU_CONTEXT(cmdlist);
    PROFILE_GPU_EVENT("Shadow");

	constexpr VkClearColorValue zeroFloat4 = VkClearColorValue{ 0.0f, 0.0f, 0.0f, 0.0f };

	// Clear values for all attachments written in the fragment shader
	VkClearValue clearValues;
	clearValues.depthStencil = { 1.0f, 0 };
	
	VkRenderPassBeginInfo renderPassBeginInfo = oGFX::vk::inits::renderPassBeginInfo();
	renderPassBeginInfo.renderPass =  renderpass_Shadow;
	renderPassBeginInfo.framebuffer = framebuffer_Shadow;
	renderPassBeginInfo.renderArea.extent.width = shadowmapSize.width;
	renderPassBeginInfo.renderArea.extent.height = shadowmapSize.height;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &clearValues;

	vkCmdBeginRenderPass(cmdlist, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	
	const float vpHeight = (float)shadowmapSize.height;
	const float vpWidth = (float)shadowmapSize.width;
	VkViewport viewport = { 0.0f, vpHeight, vpWidth, -vpHeight, 0.0f, 1.0f };
	VkRect2D scissor = { {0, 0}, {shadowmapSize.width , shadowmapSize.height } };
	vkCmdSetViewport(cmdlist, 0, 1, &viewport);
	vkCmdSetScissor(cmdlist, 0, 1, &scissor);

	vkCmdBindPipeline(cmdlist, VK_PIPELINE_BIND_POINT_GRAPHICS, pso_ShadowDefault);
	std::array<VkDescriptorSet, 3> descriptorSetGroup = 
	{
		VulkanRenderer::g0_descriptors,
		VulkanRenderer::uniformDescriptorSets[swapchainIdx],
		VulkanRenderer::globalSamplers
	};
	
	uint32_t dynamicOffset = 0;
	vkCmdBindDescriptorSets(cmdlist, VK_PIPELINE_BIND_POINT_GRAPHICS, VulkanRenderer::indirectPipeLayout,
		0, static_cast<uint32_t>(descriptorSetGroup.size()), descriptorSetGroup.data(), 1, &dynamicOffset);
	
	// Bind merged mesh vertex & index buffers, instancing buffers.
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmdlist, VERTEX_BUFFER_ID, 1, VulkanRenderer::g_MeshBuffers.VtxBuffer.getBufferPtr(), offsets);
    vkCmdBindIndexBuffer(cmdlist, VulkanRenderer::g_MeshBuffers.IdxBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindVertexBuffers(cmdlist, INSTANCE_BUFFER_ID, 1, &VulkanRenderer::instanceBuffer.buffer, offsets);

    const VkBuffer idcb = VulkanRenderer::indirectCommandsBuffer.buffer;
    const uint32_t count = (uint32_t)VulkanRenderer::m_DrawIndirectCommandsCPU.size();

	DrawIndexedIndirect(cmdlist, idcb, 0, count, sizeof(VkDrawIndexedIndirectCommand));
  
	// Other draw calls that are not supported by MDI
    // TODO: Deprecate this, or handle it gracefully. Leaving this here.
    if constexpr (false)
    {
        for (auto& entity : VulkanRenderer::entities)
        {
            auto& model = VulkanRenderer::models[entity.modelID];

            glm::mat4 xform(1.0f);
            xform = glm::translate(xform, entity.pos);
            xform = glm::rotate(xform, glm::radians(entity.rot), entity.rotVec);
            xform = glm::scale(xform, entity.scale);

            vkCmdPushConstants(cmdlist,
                VulkanRenderer::indirectPipeLayout,
                VK_SHADER_STAGE_VERTEX_BIT,	// stage to push constants to
                0,							// offset of push constants to update
                sizeof(glm::mat4),			// size of data being pushed
                glm::value_ptr(xform));		// actualy data being pushed (could be an array));

            VkDeviceSize offsets[] = { 0 };
            vkCmdBindIndexBuffer(cmdlist, VulkanRenderer::g_MeshBuffers.IdxBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);
            vkCmdBindVertexBuffers(cmdlist, VERTEX_BUFFER_ID, 1, VulkanRenderer::g_MeshBuffers.VtxBuffer.getBufferPtr(), offsets);
            vkCmdBindVertexBuffers(cmdlist, INSTANCE_BUFFER_ID, 1, &VulkanRenderer::instanceBuffer.buffer, offsets);
            //vkCmdDrawIndexed(commandBuffers[swapchainImageIndex], model.indices.count, 1, model.indices.offset, model.vertices.offset, 0);
        }
    }

	vkCmdEndRenderPass(cmdlist);
}

void ShadowPass::Shutdown()
{
	auto& m_device = VulkanRenderer::m_device;

	att_depth.destroy(m_device.logicalDevice);

	vkDestroyFramebuffer(m_device.logicalDevice, framebuffer_Shadow, nullptr);
	vkDestroyRenderPass(m_device.logicalDevice,renderpass_Shadow, nullptr);
	vkDestroyPipeline(m_device.logicalDevice, pso_ShadowDefault, nullptr);
}

void ShadowPass::SetupRenderpass()
{
	auto& m_device = VulkanRenderer::m_device;
	auto& m_swapchain = VulkanRenderer::m_swapchain;

	const uint32_t width = shadowmapSize.width;
	const uint32_t height = shadowmapSize.height;

	att_depth.createAttachment(m_device, width, height, VulkanRenderer::G_DEPTH_FORMAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

	// Set up separate renderpass with references to the color and depth attachments
	VkAttachmentDescription attachmentDescs = {};

	// Init attachment properties
	attachmentDescs.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescs.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescs.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescs.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescs.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescs.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescs.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachmentDescs.format = att_depth.format;

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 0;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.pColorAttachments = VK_NULL_HANDLE;
	subpass.colorAttachmentCount = 0;
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
	renderPassInfo.pAttachments = &attachmentDescs;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies = dependencies.data();

	VK_CHK(vkCreateRenderPass(m_device.logicalDevice, &renderPassInfo, nullptr, &renderpass_Shadow));
	VK_NAME(m_device.logicalDevice, "ShadowPass", renderpass_Shadow);
}

void ShadowPass::SetupFramebuffer()
{
	VkImageView depthView = att_depth.view;

	VkFramebufferCreateInfo fbufCreateInfo = {};
	fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbufCreateInfo.pNext = NULL;
	fbufCreateInfo.renderPass = renderpass_Shadow;
	fbufCreateInfo.pAttachments = &depthView;
	fbufCreateInfo.attachmentCount = 1;
	fbufCreateInfo.width = shadowmapSize.width;
	fbufCreateInfo.height = shadowmapSize.height;
	fbufCreateInfo.layers = 1;
	VK_CHK(vkCreateFramebuffer(VulkanRenderer::m_device.logicalDevice, &fbufCreateInfo, nullptr, &framebuffer_Shadow));
	VK_NAME(VulkanRenderer::m_device.logicalDevice, "ShadowFB", framebuffer_Shadow);

	// TODO: Fix imgui depth rendering
	//deferredImg[GBufferAttachmentIndex::DEPTH]    = ImGui_ImplVulkan_AddTexture(GfxSamplerManager::GetSampler_Deferred(), att_depth.view, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	shadowImg = ImGui_ImplVulkan_AddTexture(GfxSamplerManager::GetSampler_Deferred(), att_depth.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void ShadowPass::CreatePipeline()
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
	auto& bindingDescription = oGFX::GetGFXVertexInputBindings();

	//how the data for an attirbute is define in the vertex
	auto& attributeDescriptions = oGFX::GetGFXVertexInputAttributes();

	rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
	// -- VERTEX INPUT -- 
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = oGFX::vk::inits::pipelineVertexInputStateCreateInfo(bindingDescription,attributeDescriptions);
	//vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
	//vertexInputCreateInfo.vertexAttributeDescriptionCount = 5;

	vertexInputCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescription.size());
	vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());

	pipelineCI.pVertexInputState = &vertexInputCreateInfo;

	// Offscreen pipeline
	shaderStages[0] = VulkanRenderer::LoadShader(m_device, "Shaders/bin/shadow.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = VulkanRenderer::LoadShader(m_device, "Shaders/bin/shadow.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	// Separate render pass
	pipelineCI.renderPass = renderpass_Shadow;

	// Blend attachment states required for all color attachments
	// This is important, as color write mask will otherwise be 0x0 and you
	// won't see anything rendered to the attachment
	
	//std::array<VkPipelineColorBlendAttachmentState, GBufferAttachmentIndex::TOTAL_COLOR_ATTACHMENTS> blendAttachmentStates =
	//{
	//	oGFX::vk::inits::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
	//	oGFX::vk::inits::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
	//	oGFX::vk::inits::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
	//	oGFX::vk::inits::pipelineColorBlendAttachmentState(0xf, VK_FALSE)
	//};
	//
	//colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
	//colorBlendState.pAttachments = blendAttachmentStates.data();

	VK_CHK(vkCreateGraphicsPipelines(m_device.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pso_ShadowDefault));
	VK_NAME(m_device.logicalDevice, "ShadowPipline", pso_ShadowDefault);
	vkDestroyShaderModule(m_device.logicalDevice, shaderStages[0].module, nullptr);
	vkDestroyShaderModule(m_device.logicalDevice, shaderStages[1].module, nullptr);

}
