#include "ForwardDecalRenderpass.h"

#include "VulkanRenderer.h"

#include "GBufferRenderPass.h"
#include "DebugDraw.h"

class DecalInstanceWrapper
{
public:

	void Init(const DecalInstance& decalInstance)
	{
		glm::vec3 position = decalInstance.position;
		glm::vec3 decalProjectDirection = decalInstance.direction;
		float nearZ = decalInstance.nearZ;
		float farZ = decalInstance.farZ;
		float testVar0 = decalInstance.testVar0;
		float projectorSize = decalInstance.projectorSize;

		const glm::vec3 projector_dir = decalProjectDirection;
		const glm::vec3 projectorPosition = position - projector_dir * testVar0;
		m_ProjectorPosition = projectorPosition;
		m_ProjectorDirection = decalProjectDirection;

		glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), glm::radians(decalInstance.rotationDegrees), m_ProjectorDirection);
		glm::vec3 default_up = glm::vec3(0.0f, 0.0f, 1.0f);
		glm::vec4 rotatedAxis = rotate * glm::vec4(default_up, 0.0f);
		// TODO: There will be edge case if the direction is similar to the default up vector.

		// This only determines the decal aspect ratio
		glm::vec2 decalDimensions{ 32.0f, 32.0f }; // From the decal instance
		glm::vec2 projectorAspectRatio{ 1.0f,1.0f };

		if (decalDimensions.x > decalDimensions.y)
		{
			projectorAspectRatio.x = 1.0f;
			projectorAspectRatio.y = decalDimensions.x / decalDimensions.y;
		}
		else
		{
			projectorAspectRatio.x = decalDimensions.y / decalDimensions.x;
			projectorAspectRatio.y = 1.0f;
		}
		// TODO: See if aspect ratio is needed...

		m_View = glm::lookAt(projectorPosition, position, glm::normalize(glm::vec3(rotatedAxis) + glm::vec3(0.001f, 0.0f, 0.0f)));
		m_Projection = glm::ortho(-projectorSize, projectorSize, -projectorSize, projectorSize, nearZ, farZ);
		m_ViewProjection = m_Projection * m_View;
	}

	void DebugDraw()
	{
		// Debug Draw some shit...
		{
			DebugDraw::AddLine(m_ProjectorPosition, m_ProjectorPosition + m_ProjectorDirection, oGFX::Colors::BLUE);
			AABB aabb;
			aabb.halfExt = { 0.1f, 0.1f ,0.1f };
			aabb.center = m_ProjectorPosition;
			// Position of the decal projector
			DebugDraw::AddAABB(aabb, oGFX::Colors::BLUE);

			// Taken from the decal shader
#define BOUND 0.5f
			constexpr static glm::vec3 vertices[36] =
			{
				{-BOUND,-BOUND,-BOUND}, // [ 0] // -X
				{-BOUND,-BOUND, BOUND}, // [ 1] 
				{-BOUND, BOUND, BOUND}, // [ 2] 
				{-BOUND, BOUND, BOUND}, // [ 3] 
				{-BOUND, BOUND,-BOUND}, // [ 4] 
				{-BOUND,-BOUND,-BOUND}, // [ 5] 

				{-BOUND,-BOUND,-BOUND}, // [ 6] // -Z
				{ BOUND, BOUND,-BOUND}, // [ 7] 
				{ BOUND,-BOUND,-BOUND}, // [ 8] 
				{-BOUND,-BOUND,-BOUND}, // [ 9] 
				{-BOUND, BOUND,-BOUND}, // [10] 
				{ BOUND, BOUND,-BOUND}, // [11] 

				{-BOUND,-BOUND,-BOUND}, // [12] // -Y
				{ BOUND,-BOUND,-BOUND}, // [13] 
				{ BOUND,-BOUND, BOUND}, // [14] 
				{-BOUND,-BOUND,-BOUND}, // [15] 
				{ BOUND,-BOUND, BOUND}, // [16] 
				{-BOUND,-BOUND, BOUND}, // [17] 

				{-BOUND, BOUND,-BOUND}, // [18] // +Y
				{-BOUND, BOUND, BOUND}, // [19] 
				{ BOUND, BOUND, BOUND}, // [20] 
				{-BOUND, BOUND,-BOUND}, // [21] 
				{ BOUND, BOUND, BOUND}, // [22] 
				{ BOUND, BOUND,-BOUND}, // [23] 

				{ BOUND, BOUND,-BOUND}, // [24] // +X
				{ BOUND, BOUND, BOUND}, // [25] 
				{ BOUND,-BOUND, BOUND}, // [26] 
				{ BOUND,-BOUND, BOUND}, // [27] 
				{ BOUND,-BOUND,-BOUND}, // [28] 
				{ BOUND, BOUND,-BOUND}, // [29] 

				{-BOUND, BOUND, BOUND}, // [30] // +Z
				{-BOUND,-BOUND, BOUND}, // [31] 
				{ BOUND, BOUND, BOUND}, // [32] 
				{-BOUND,-BOUND, BOUND}, // [33] 
				{ BOUND,-BOUND, BOUND}, // [34] 
				{ BOUND, BOUND, BOUND}, // [35] 
			};
#undef BOUND

			static std::array<glm::vec3, 36> s_worldPositions;

			auto g_InvDecalViewProjection = glm::inverse(m_ViewProjection);

			aabb.halfExt = { 0.01f, 0.01f ,0.01f };
			for (int i = 0; i < 36; ++i)
			{
				glm::vec4 worldPosition = g_InvDecalViewProjection * glm::vec4(vertices[i], 1.0f);
				s_worldPositions[i] = worldPosition;
				aabb.center = worldPosition;
				// Draw the vertices after transforming them to the world space.
				DebugDraw::AddAABB(aabb, oGFX::Colors::GREEN);
			}

			// Draw the BV for decal volume
			aabb.center = 0.5f * (s_worldPositions[0] + s_worldPositions[35]);
			aabb.halfExt = 0.5f * (s_worldPositions[35] - s_worldPositions[0]);
			//DebugDraw::AddAABB(aabb, oGFX::Colors::INDIGO);
			// -X
			DebugDraw::AddLine(s_worldPositions[0], s_worldPositions[1], oGFX::Colors::INDIGO);
			DebugDraw::AddLine(s_worldPositions[0], s_worldPositions[4], oGFX::Colors::INDIGO);
			DebugDraw::AddLine(s_worldPositions[2], s_worldPositions[1], oGFX::Colors::INDIGO);
			DebugDraw::AddLine(s_worldPositions[2], s_worldPositions[4], oGFX::Colors::INDIGO);
			// -Z
			DebugDraw::AddLine(s_worldPositions[6], s_worldPositions[8], oGFX::Colors::INDIGO);
			DebugDraw::AddLine(s_worldPositions[6], s_worldPositions[10], oGFX::Colors::INDIGO);
			DebugDraw::AddLine(s_worldPositions[11], s_worldPositions[8], oGFX::Colors::INDIGO);
			DebugDraw::AddLine(s_worldPositions[11], s_worldPositions[10], oGFX::Colors::INDIGO);
			// -Y
			DebugDraw::AddLine(s_worldPositions[12], s_worldPositions[13], oGFX::Colors::INDIGO);
			DebugDraw::AddLine(s_worldPositions[12], s_worldPositions[17], oGFX::Colors::INDIGO);
			DebugDraw::AddLine(s_worldPositions[14], s_worldPositions[13], oGFX::Colors::INDIGO);
			DebugDraw::AddLine(s_worldPositions[14], s_worldPositions[17], oGFX::Colors::INDIGO);
			// +Y
			DebugDraw::AddLine(s_worldPositions[18], s_worldPositions[19], oGFX::Colors::INDIGO);
			DebugDraw::AddLine(s_worldPositions[18], s_worldPositions[23], oGFX::Colors::INDIGO);
			DebugDraw::AddLine(s_worldPositions[20], s_worldPositions[19], oGFX::Colors::INDIGO);
			DebugDraw::AddLine(s_worldPositions[20], s_worldPositions[23], oGFX::Colors::INDIGO);
			// +X
			DebugDraw::AddLine(s_worldPositions[28], s_worldPositions[24], oGFX::Colors::INDIGO);
			DebugDraw::AddLine(s_worldPositions[28], s_worldPositions[26], oGFX::Colors::INDIGO);
			DebugDraw::AddLine(s_worldPositions[25], s_worldPositions[24], oGFX::Colors::INDIGO);
			DebugDraw::AddLine(s_worldPositions[25], s_worldPositions[26], oGFX::Colors::INDIGO);
			// +Z
			DebugDraw::AddLine(s_worldPositions[31], s_worldPositions[30], oGFX::Colors::INDIGO);
			DebugDraw::AddLine(s_worldPositions[31], s_worldPositions[34], oGFX::Colors::INDIGO);
			DebugDraw::AddLine(s_worldPositions[35], s_worldPositions[30], oGFX::Colors::INDIGO);
			DebugDraw::AddLine(s_worldPositions[35], s_worldPositions[34], oGFX::Colors::INDIGO);
		}
	}

	glm::mat4x4 GetViewProjection() { return m_ViewProjection; }

private:
	glm::vec3 m_ProjectorPosition;
	glm::vec3 m_ProjectorDirection;
	glm::mat4x4 m_View;
	glm::mat4x4 m_Projection;
	glm::mat4x4 m_ViewProjection;
};

DECLARE_RENDERPASS(ForwardDecalRenderpass);

void ForwardDecalRenderpass::Init()
{
    SetupRenderpass();
    SetupFramebuffer();
}

void ForwardDecalRenderpass::CreatePSO()
{
    CreateDescriptorLayout();
    CreatePipeline();
}

void ForwardDecalRenderpass::Draw()
{
    auto& vr = *VulkanRenderer::get();
    
    auto& device = vr.m_device;
    auto& swapchain = vr.m_swapchain;
    auto& commandBuffers = vr.commandBuffers;
    auto& swapchainIdx = vr.swapchainIdx;
    auto* windowPtr = vr.windowPtr;

    const VkCommandBuffer cmdlist = commandBuffers[swapchainIdx];
    PROFILE_GPU_CONTEXT(cmdlist);
    PROFILE_GPU_EVENT("ForwardDecal");

    constexpr VkClearColorValue zeroFloat4 = VkClearColorValue{ 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = zeroFloat4;
    clearValues[1].depthStencil.depth = { 1.0f };

    VkRenderPassBeginInfo renderPassBeginInfo = oGFX::vkutils::inits::renderPassBeginInfo(clearValues);
    renderPassBeginInfo.renderPass = renderpass_ForwardDecal;
    renderPassBeginInfo.renderArea.extent = vr.m_swapchain.swapChainExtent;
    renderPassBeginInfo.framebuffer = vr.swapChainFramebuffers[swapchainIdx];

    vkCmdBeginRenderPass(cmdlist, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    rhi::CommandList cmd{ cmdlist };
    
    cmd.SetDefaultViewportAndScissor();
    cmd.BindPSO(pso_ForwardDecalDefault);

	cmd.BindDescriptorSet(PSOLayoutDB::deferredLightingCompositionPSOLayout, 0,
		std::array<VkDescriptorSet, 2>
	    {
		    vr.descriptorSet_DeferredComposition,
            vr.descriptorSets_uniform[swapchainIdx]
	    }
	);

	// For each decal
	{
		auto& decalInstance = vr.currWorld->m_HardcodedDecalInstance;
		if (decalInstance.active)
		{
			DecalInstanceWrapper helper;
			helper.Init(decalInstance);
			helper.DebugDraw();

			struct {
				glm::mat4 g_InvDecalViewProjection;
				glm::mat4 g_DecalViewProjection;
			}pc;

			pc.g_InvDecalViewProjection = glm::inverse(helper.GetViewProjection());
			pc.g_DecalViewProjection = helper.GetViewProjection();

			vkCmdPushConstants(cmdlist, PSOLayoutDB::deferredLightingCompositionPSOLayout, VK_SHADER_STAGE_ALL, 0, sizeof(glm::mat4) * 2, &pc);
			cmd.Draw(36, 1);
		}
	}

    vkCmdEndRenderPass(cmdlist);
}

void ForwardDecalRenderpass::Shutdown()
{
}

void ForwardDecalRenderpass::SetupRenderpass()
{
	auto& vr = *VulkanRenderer::get();
	VkAttachmentDescription colourAttachment = {};
	colourAttachment.format = vr.m_swapchain.swapChainImageFormat;
	colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	colourAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colourAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Depth attachment of render pass
	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = vr.G_DEPTH_FORMAT;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference  colourAttachmentReference = {};
	colourAttachmentReference.attachment = 0;
	colourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentReference{};
	depthAttachmentReference.attachment = 1;
	depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	//information about a particular subpass the render pass is using
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; //pipeline type subpass is to be bound to
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colourAttachmentReference;
	subpass.pDepthStencilAttachment = &depthAttachmentReference;

	// Need to determine when layout transitions occur using subpass dependancies
	std::array<VkSubpassDependency, 2> subpassDependancies;

	subpassDependancies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependancies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependancies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependancies[0].dstSubpass = 0;
	subpassDependancies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependancies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependancies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	
	subpassDependancies[1].srcSubpass = 0;
	subpassDependancies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependancies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependancies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependancies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependancies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependancies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	std::array<VkAttachmentDescription, 2> renderpassAttachments =
    {
        colourAttachment, depthAttachment 
    };

	//create info for render pass
	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(renderpassAttachments.size());
	renderPassCreateInfo.pAttachments = renderpassAttachments.data();
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependancies.size());
	renderPassCreateInfo.pDependencies = subpassDependancies.data();

	VK_CHK(vkCreateRenderPass(vr.m_device.logicalDevice, &renderPassCreateInfo, nullptr, &renderpass_ForwardDecal));
	VK_NAME(vr.m_device.logicalDevice, "renderpass_ForwardDecal", renderpass_ForwardDecal);
}

void ForwardDecalRenderpass::SetupFramebuffer()
{
    // ??
}

void ForwardDecalRenderpass::CreateDescriptorLayout()
{
    return;
    auto gbuffer = RenderPassDatabase::GetRenderPass<GBufferRenderPass>();
    assert(gbuffer != nullptr);
	auto& vr = *VulkanRenderer::get();

    VkDescriptorImageInfo texDescriptorDepth = oGFX::vkutils::inits::descriptorImageInfo(
        GfxSamplerManager::GetSampler_Deferred(),
        gbuffer->attachments[GBufferAttachmentIndex::DEPTH].view,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    DescriptorBuilder::Begin(&vr.DescLayoutCache, &vr.descAllocs[vr.swapchainIdx])
        .BindImage(3, &texDescriptorDepth, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .Build(this->descriptorSet_ForwardDecal, SetLayoutDB::ForwardDecal);
}

void ForwardDecalRenderpass::CreatePipelineLayout()
{
    return;
	auto& vr = *VulkanRenderer::get();
	auto& m_device = vr.m_device;

	std::vector<VkDescriptorSetLayout> setLayouts
	{
		SetLayoutDB::ForwardDecal // (set = 0)
	};

	VkPipelineLayoutCreateInfo plci = oGFX::vkutils::inits::pipelineLayoutCreateInfo(setLayouts.data(), static_cast<uint32_t>(setLayouts.size()));
	VkPushConstantRange pushConstantRange{ VK_SHADER_STAGE_ALL, 0, 128 };
	plci.pushConstantRangeCount = 1;
	plci.pPushConstantRanges = &pushConstantRange;

	VK_CHK(vkCreatePipelineLayout(m_device.logicalDevice, &plci, nullptr, &PSOLayoutDB::forwardDecalPSOLayout));
	VK_NAME(m_device.logicalDevice, "forwardDecalPSOLayout", PSOLayoutDB::forwardDecalPSOLayout);
}

void ForwardDecalRenderpass::CreatePipeline()
{
    auto& vr = *VulkanRenderer::get();

    auto& device = vr.m_device.logicalDevice;

	const char* shaderVS = "Shaders/bin/forwarddecal.vert.spv";
	const char* shaderPS = "Shaders/bin/forwarddecal.frag.spv";
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages =
	{
		VulkanRenderer::LoadShader(vr.m_device, shaderVS, VK_SHADER_STAGE_VERTEX_BIT),
		VulkanRenderer::LoadShader(vr.m_device, shaderPS, VK_SHADER_STAGE_FRAGMENT_BIT)
	};

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = oGFX::vkutils::inits::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
    VkPipelineRasterizationStateCreateInfo rasterizationState = oGFX::vkutils::inits::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_CLOCKWISE);
    VkPipelineColorBlendAttachmentState blendAttachmentState = oGFX::vkutils::inits::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
    VkPipelineColorBlendStateCreateInfo colorBlendState = oGFX::vkutils::inits::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
    VkPipelineDepthStencilStateCreateInfo depthStencilState = oGFX::vkutils::inits::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_FALSE, VK_COMPARE_OP_GREATER_OR_EQUAL);//VK_COMPARE_OP_GREATER_OR_EQUAL,VK_COMPARE_OP_LESS_OR_EQUAL
    VkPipelineViewportStateCreateInfo viewportState = oGFX::vkutils::inits::pipelineViewportStateCreateInfo(1, 1, 0);
    VkPipelineMultisampleStateCreateInfo multisampleState = oGFX::vkutils::inits::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
    std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState = oGFX::vkutils::inits::pipelineDynamicStateCreateInfo(dynamicStateEnables);

    VkGraphicsPipelineCreateInfo pipelineCI = oGFX::vkutils::inits::pipelineCreateInfo(PSOLayoutDB::deferredLightingCompositionPSOLayout, renderpass_ForwardDecal);
    pipelineCI.pInputAssemblyState = &inputAssemblyState;
    pipelineCI.pRasterizationState = &rasterizationState;
    pipelineCI.pColorBlendState = &colorBlendState;
    pipelineCI.pMultisampleState = &multisampleState;
    pipelineCI.pViewportState = &viewportState;
    pipelineCI.pDepthStencilState = &depthStencilState;
    pipelineCI.pDynamicState = &dynamicState;
    pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCI.pStages = shaderStages.data();

    auto& bindingDescription = oGFX::GetGFXVertexInputBindings();
    auto& attributeDescriptions = oGFX::GetGFXVertexInputAttributes();
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = oGFX::vkutils::inits::pipelineVertexInputStateCreateInfo(bindingDescription, attributeDescriptions);
    pipelineCI.pVertexInputState = &vertexInputCreateInfo;
    
    pipelineCI.renderPass = renderpass_ForwardDecal;

    VK_CHK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pso_ForwardDecalDefault));
    VK_NAME(device, "ForwardDecalPSO", pso_ForwardDecalDefault);
    vkDestroyShaderModule(device, shaderStages[0].module, nullptr);
    vkDestroyShaderModule(device, shaderStages[1].module, nullptr);
}
