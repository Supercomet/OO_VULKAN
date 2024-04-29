/************************************************************************************//*!
\file           ForwardUIPass.cpp
\project        Ouroboros
\author         Jamie Kong, j.kong, 390004720 | code contribution (100%)
\par            email: j.kong\@digipen.edu
\date           Oct 02, 2022
\brief              Defines a gbuffer pass

Copyright (C) 2022 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*//*************************************************************************************/
#include "GfxRenderpass.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_vulkan.h"

#include "Window.h"
#include "VulkanRenderer.h"
#include "VulkanUtils.h"
#include "FramebufferCache.h"
#include "FramebufferBuilder.h"

#include "../shaders/shared_structs.h"
#include "MathCommon.h"

#include "GraphicsWorld.h"

#include <array>


struct ForwardUIPass : public GfxRenderpass
{
	//DECLARE_RENDERPASS_SINGLETON(ForwardUIPass)
	ForwardUIPass(const char* _name) : GfxRenderpass{ _name } {}

	void Init() override;
	void Draw(const VkCommandBuffer cmdlist) override;
	void Shutdown() override;

	bool SetupDependencies(RenderGraph& builder) override;

	void CreatePSO() override;

private:
	void SetupRenderpass();
	void SetupFramebuffer();
	void CreatePipeline();

};

DECLARE_RENDERPASS(ForwardUIPass);


//VkPushConstantRange pushConstantRange;
VkPipeline pso_Forward_UI{};


void ForwardUIPass::Init()
{
	SetupRenderpass();
	SetupFramebuffer();
}

void ForwardUIPass::CreatePSO()
{
	CreatePipeline();
}

bool ForwardUIPass::SetupDependencies(RenderGraph& builder)
{
	auto& vr = *VulkanRenderer::get();
	// TODO: If gbuffer rendering is disabled, return false.
	builder.Write(vr.attachments.lighting_target, ATTACHMENT);
	builder.Write(vr.attachments.gbuffer[GBufferAttachmentIndex::VELOCITY], ATTACHMENT);
	builder.Write(vr.attachments.gbuffer[GBufferAttachmentIndex::ENTITY_ID], ATTACHMENT);
	builder.Write(vr.attachments.gbuffer[GBufferAttachmentIndex::DEPTH], ATTACHMENT);

	builder.Read(vr.g_UIVertexBufferGPU);
	builder.Read(vr.g_UIIndexBufferGPU);
	// READ: Scene data SSBO
	// READ: Instancing Data
	// READ: Bindless stuff
	// WRITE: GBuffer Albedo
	// WRITE: GBuffer Normal
	// WRITE: GBuffer Material
	// WRITE: GBuffer Depth
	// etc
	
	return true;
}

void ForwardUIPass::Draw(const VkCommandBuffer cmdlist)
{
	auto& vr = *VulkanRenderer::get();
	lastCmd = cmdlist;
	if (!vr.deferredRendering)
		return;
	auto& device = vr.m_device;
	auto& swapchain = vr.m_swapchain;
	auto currFrame = vr.getFrame();
	auto* windowPtr = vr.windowPtr;

	PROFILE_GPU_CONTEXT(cmdlist);
    PROFILE_GPU_EVENT("ForwardUI");

	rhi::CommandList cmd{ cmdlist, "Forward UI Pass"};
	
	auto& attachments = vr.attachments.gbuffer;
	auto& target = vr.attachments.lighting_target;

	cmd.BindAttachment(0, &target);
	cmd.BindAttachment(1, &vr.attachments.gbuffer[VELOCITY]);
	cmd.BindAttachment(2, &attachments[GBufferAttachmentIndex::ENTITY_ID]);
	cmd.BindDepthAttachment(&attachments[GBufferAttachmentIndex::DEPTH]);


	cmd.BindPSO(pso_Forward_UI, PSOLayoutDB::defaultPSOLayout);
	cmd.SetDefaultViewportAndScissor();
	uint32_t dynamicOffset = static_cast<uint32_t>(vr.renderIteration * oGFX::vkutils::tools::UniformBufferPaddedSize(sizeof(CB::FrameContextUBO), 
												vr.m_device.properties.limits.minUniformBufferOffsetAlignment));
	
	cmd.BindDescriptorSet(PSOLayoutDB::defaultPSOLayout, 0, 
		std::array<VkDescriptorSet, 3>{
		vr.descriptorSet_gpuscene,
			vr.descriptorSets_uniform[currFrame],
			vr.descriptorSet_bindless,
	},
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		1, & dynamicOffset
	);

	// Bind merged mesh vertex & index buffers, instancing buffers.
	std::vector<VkBuffer> vtxBuffers{
		vr.g_UIVertexBufferGPU.getBuffer(),
	};

	VkDeviceSize offsets[2]{
		0,
		0
	};
	cmd.BindVertexBuffer(BIND_POINT_VERTEX_BUFFER_ID, 1, vr.g_UIVertexBufferGPU.getBufferPtr());
	cmd.BindIndexBuffer(vr.g_UIIndexBufferGPU.getBuffer(), 0, VK_INDEX_TYPE_UINT32);
	
	//cmd.BindVertexBuffer(BIND_POINT_INSTANCE_BUFFER_ID, 1, vr.g_particleDatas.getBufferPtr());
	
	auto &uivert = vr.batches.GetUIVertices();
	const auto ScreenSpaceVtxOffset = vr.batches.GetScreenSpaceUIOffset();

	const auto instanceCnt = uivert.size() / 4;
	const auto indicesCnt =  instanceCnt* 6;

	const auto ScreenSpaceCnt = instanceCnt - (ScreenSpaceVtxOffset / 4);
	const auto ScreenSpaceIndices = ScreenSpaceCnt * 6;
	const auto WorldSpaceCnt = instanceCnt - ScreenSpaceCnt;
	const auto WorldSpaceIndices = WorldSpaceCnt * 6;
	const auto ScreenSpaceIdxOffset = WorldSpaceIndices;

	// do draw command here
	// batch draw from the big buffer
	cmd.DrawIndexed(static_cast<uint32_t>(WorldSpaceIndices), 1);// draw worldspace
	

}

void ForwardUIPass::Shutdown()
{
	auto& device = VulkanRenderer::get()->m_device.logicalDevice;

	vkDestroyPipeline(device, pso_Forward_UI, nullptr);
}

void ForwardUIPass::SetupRenderpass()
{
	auto& vr = *VulkanRenderer::get();
	auto& m_device = vr.m_device;
	auto& m_swapchain = vr.m_swapchain;

	const uint32_t width = m_swapchain.swapChainExtent.width;
	const uint32_t height = m_swapchain.swapChainExtent.height;

}

void ForwardUIPass::SetupFramebuffer()
{
	auto& vr = *VulkanRenderer::get();
	auto& m_device = vr.m_device;
	auto& m_swapchain = vr.m_swapchain;

	const uint32_t width = m_swapchain.swapChainExtent.width;
	const uint32_t height = m_swapchain.swapChainExtent.height;

	// maybe dont use this??
	auto& attachments = vr.attachments.gbuffer;

}


void ForwardUIPass::CreatePipeline()
{
	auto& vr = *VulkanRenderer::get();
	auto& m_device = vr.m_device;

	const char* shaderVS = "Shaders/bin/forwardUI.vert.spv";
	const char* shaderPS = "Shaders/bin/forwardUI.frag.spv";
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages =
	{
		vr.LoadShader(m_device, shaderVS, VK_SHADER_STAGE_VERTEX_BIT),
		vr.LoadShader(m_device, shaderPS, VK_SHADER_STAGE_FRAGMENT_BIT)
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = oGFX::vkutils::inits::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	VkPipelineRasterizationStateCreateInfo rasterizationState = oGFX::vkutils::inits::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	VkPipelineColorBlendAttachmentState blendAttachmentState = oGFX::vkutils::inits::pipelineColorBlendAttachmentState(0xf, VK_TRUE); // we want blending 
	VkPipelineColorBlendStateCreateInfo colorBlendState = oGFX::vkutils::inits::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
	VkPipelineDepthStencilStateCreateInfo depthStencilState = oGFX::vkutils::inits::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, vr.G_DEPTH_COMPARISON);
	VkPipelineViewportStateCreateInfo viewportState = oGFX::vkutils::inits::pipelineViewportStateCreateInfo(1, 1, 0);
	VkPipelineMultisampleStateCreateInfo multisampleState = oGFX::vkutils::inits::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
	std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamicState = oGFX::vkutils::inits::pipelineDynamicStateCreateInfo(dynamicStateEnables);

	VkGraphicsPipelineCreateInfo pipelineCI = oGFX::vkutils::inits::pipelineCreateInfo(PSOLayoutDB::defaultPSOLayout, VK_NULL_HANDLE);
	pipelineCI.pInputAssemblyState = &inputAssemblyState;
	pipelineCI.pRasterizationState = &rasterizationState;
	pipelineCI.pColorBlendState = &colorBlendState;
	pipelineCI.pMultisampleState = &multisampleState;
	pipelineCI.pViewportState = &viewportState;
	pipelineCI.pDepthStencilState = &depthStencilState;
	pipelineCI.pDynamicState = &dynamicState;
	pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineCI.pStages = shaderStages.data();

	const auto& bindingDescription = std::vector<VkVertexInputBindingDescription>{	
		oGFX::vkutils::inits::vertexInputBindingDescription(BIND_POINT_VERTEX_BUFFER_ID,sizeof(oGFX::UIVertex),VK_VERTEX_INPUT_RATE_VERTEX),
		//oGFX::vkutils::inits::vertexInputBindingDescription(BIND_POINT_INSTANCE_BUFFER_ID,sizeof(UIData),VK_VERTEX_INPUT_RATE_INSTANCE),
	};
	const auto& attributeDescriptions = std::vector<VkVertexInputAttributeDescription>{
		oGFX::vkutils::inits::vertexInputAttributeDescription(BIND_POINT_VERTEX_BUFFER_ID,0,VK_FORMAT_R32G32B32A32_SFLOAT,offsetof(oGFX::UIVertex, pos)), //Position attribute
		oGFX::vkutils::inits::vertexInputAttributeDescription(BIND_POINT_VERTEX_BUFFER_ID,1,VK_FORMAT_R32G32B32A32_SFLOAT,offsetof(oGFX::UIVertex, tex)),    //Texture attribute
		oGFX::vkutils::inits::vertexInputAttributeDescription(BIND_POINT_VERTEX_BUFFER_ID,2,VK_FORMAT_R32G32B32A32_SFLOAT,offsetof(oGFX::UIVertex, col)),    //Texture attribute


		//oGFX::vkutils::inits::vertexInputAttributeDescription(BIND_POINT_INSTANCE_BUFFER_ID,2,VK_FORMAT_R32G32B32A32_SFLOAT,offsetof(UIData, transform)+0*sizeof(glm::vec4)), //xform
		//oGFX::vkutils::inits::vertexInputAttributeDescription(BIND_POINT_INSTANCE_BUFFER_ID,3,VK_FORMAT_R32G32B32A32_SFLOAT,offsetof(UIData, transform)+1*sizeof(glm::vec4)), //xform
		//oGFX::vkutils::inits::vertexInputAttributeDescription(BIND_POINT_INSTANCE_BUFFER_ID,4,VK_FORMAT_R32G32B32A32_SFLOAT,offsetof(UIData, transform)+2*sizeof(glm::vec4)), //xform
		//oGFX::vkutils::inits::vertexInputAttributeDescription(BIND_POINT_INSTANCE_BUFFER_ID,5,VK_FORMAT_R32G32B32A32_SFLOAT,offsetof(UIData, transform)+3*sizeof(glm::vec4)), //xform
		//oGFX::vkutils::inits::vertexInputAttributeDescription(BIND_POINT_INSTANCE_BUFFER_ID,6,VK_FORMAT_R32G32B32A32_UINT,offsetof(UIData, instanceData)),    //texindex, entityID

	};
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = oGFX::vkutils::inits::pipelineVertexInputStateCreateInfo(bindingDescription, attributeDescriptions);
	pipelineCI.pVertexInputState = &vertexInputCreateInfo;

	// Separate render pass
	// pipelineCI.renderPass = renderpass_ForwardUI.pass;
	pipelineCI.renderPass = VK_NULL_HANDLE;

	// Blend attachment states required for all color attachments
	// This is important, as color write mask will otherwise be 0x0 and you
	// won't see anything rendered to the attachment
	std::array<VkPipelineColorBlendAttachmentState, 3> blendAttachmentStates =
	{
		oGFX::vkutils::inits::pipelineColorBlendAttachmentState(0xF, VK_FALSE),
		oGFX::vkutils::inits::pipelineColorBlendAttachmentState(0xF, VK_FALSE),
		oGFX::vkutils::inits::pipelineColorBlendAttachmentState(0xF, VK_FALSE),
		//oGFX::vkutils::inits::pipelineColorBlendAttachmentState(0xf, VK_FALSE), // albedo blend
		//oGFX::vkutils::inits::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
		//oGFX::vkutils::inits::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
		//oGFX::vkutils::inits::pipelineColorBlendAttachmentState(0xf, VK_FALSE)
	};
	blendAttachmentStates[0].blendEnable = VK_TRUE; 
	blendAttachmentStates[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendAttachmentStates[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendAttachmentStates[0].colorBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentStates[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendAttachmentStates[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // save background albedo as well
	blendAttachmentStates[0].alphaBlendOp = VK_BLEND_OP_ADD;

	colorBlendState.attachmentCount = (uint32_t)blendAttachmentStates.size();
	colorBlendState.pAttachments = blendAttachmentStates.data();

	std::array<VkFormat, 3> formats{
		vr.G_HDR_FORMAT,
		vr.G_VELOCITY_FORMAT,
		VK_FORMAT_R32_SINT
	};
	VkPipelineRenderingCreateInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	renderingInfo.viewMask = {};
	renderingInfo.colorAttachmentCount = (uint32_t)formats.size();
	renderingInfo.pColorAttachmentFormats = formats.data();
	renderingInfo.depthAttachmentFormat = vr.G_DEPTH_FORMAT;
	renderingInfo.stencilAttachmentFormat = vr.G_DEPTH_FORMAT;

	pipelineCI.pNext = &renderingInfo;

	if (pso_Forward_UI != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(m_device.logicalDevice, pso_Forward_UI, nullptr);
	}
	VK_CHK(vkCreateGraphicsPipelines(m_device.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pso_Forward_UI));
	VK_NAME(m_device.logicalDevice, "forwardUIPSO", pso_Forward_UI);


	vkDestroyShaderModule(m_device.logicalDevice, shaderStages[0].module, nullptr);
	vkDestroyShaderModule(m_device.logicalDevice, shaderStages[1].module, nullptr);
}
