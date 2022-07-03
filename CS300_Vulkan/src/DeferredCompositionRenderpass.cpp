#include "DeferredCompositionRenderpass.h"

#include "VulkanRenderer.h"
#include "Window.h"
#include "VulkanUtils.h"

#include <array>

DECLARE_RENDERPASS(DeferredCompositionRenderpass);

void DeferredCompositionRenderpass::Init()
{
	//CreatePipeline(); // Dependency on GBuffer Init()
}

void DeferredCompositionRenderpass::Draw()
{
	auto swapchainIdx = VulkanRenderer::swapchainIdx;
	auto* windowPtr = VulkanRenderer::windowPtr;
	std::array<VkClearValue, 2> clearValues{};
	//clearValues[0].color = { 0.6f,0.65f,0.4f,1.0f };
	clearValues[0].color = { 0.1f,0.1f,0.1f,1.0f };
	clearValues[1].depthStencil.depth = { 1.0f };

	//Information about how to begin a render pass (only needed for graphical applications)
	VkRenderPassBeginInfo renderPassBeginInfo = oGFX::vk::inits::renderPassBeginInfo();
	renderPassBeginInfo.renderPass = VulkanRenderer::defaultRenderPass;                  //render pass to begin
	renderPassBeginInfo.renderArea.offset = { 0,0 };                                     //start point of render pass in pixels
	renderPassBeginInfo.renderArea.extent = VulkanRenderer::m_swapchain.swapChainExtent; //size of region to run render pass on (Starting from offset)
	renderPassBeginInfo.pClearValues = clearValues.data();                               //list of clear values
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());

	renderPassBeginInfo.framebuffer =  VulkanRenderer::swapChainFramebuffers[swapchainIdx];

	const VkCommandBuffer cmdlist = VulkanRenderer::commandBuffers[swapchainIdx];

	vkCmdBeginRenderPass(cmdlist, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	SetDefaultViewportAndScissor(cmdlist);

	vkCmdBindPipeline(cmdlist, VK_PIPELINE_BIND_POINT_GRAPHICS, compositionPipe);
	std::array<VkDescriptorSet, 2> descriptorSetGroup = 
	{
		VulkanRenderer::uniformDescriptorSets[swapchainIdx],
		VulkanRenderer::globalSamplers
	};

	vkCmdBindDescriptorSets(cmdlist, VK_PIPELINE_BIND_POINT_GRAPHICS, compositionPipeLayout, 0, 1, &VulkanRenderer::deferredSet, 0, nullptr);

	DrawFullScreenQuad(cmdlist);

	vkCmdEndRenderPass(cmdlist);
}

void DeferredCompositionRenderpass::Shutdown()
{
	vkDestroyPipelineLayout(VulkanRenderer::m_device.logicalDevice, compositionPipeLayout, nullptr);
	vkDestroyRenderPass(VulkanRenderer::m_device.logicalDevice,compositionPass, nullptr);
	vkDestroyPipeline(VulkanRenderer::m_device.logicalDevice, compositionPipe, nullptr);
}

void DeferredCompositionRenderpass::CreatePipeline()
{
	auto& m_device = VulkanRenderer::m_device;

	std::vector<VkDescriptorSetLayout> setLayouts{ VulkanRenderer::deferredSetLayout };

	VkPipelineLayoutCreateInfo plci = oGFX::vk::inits::pipelineLayoutCreateInfo(setLayouts.data(),static_cast<uint32_t>(setLayouts.size()));	
	plci.pushConstantRangeCount = 1;
	plci.pPushConstantRanges = &VulkanRenderer::pushConstantRange;

	VK_CHK(vkCreatePipelineLayout(m_device.logicalDevice, &plci, nullptr, &compositionPipeLayout));
	VK_NAME(m_device.logicalDevice, "compositionPipeLayout", compositionPipeLayout);

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = oGFX::vk::inits::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
	VkPipelineRasterizationStateCreateInfo rasterizationState = oGFX::vk::inits::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
	VkPipelineColorBlendAttachmentState blendAttachmentState = oGFX::vk::inits::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
	VkPipelineColorBlendStateCreateInfo colorBlendState = oGFX::vk::inits::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
	VkPipelineDepthStencilStateCreateInfo depthStencilState = oGFX::vk::inits::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
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
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions {
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

	vertexInputCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescription.size());
	vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());

	pipelineCI.pVertexInputState = &vertexInputCreateInfo;

	// Empty vertex input state, vertices are generated by the vertex shader
	// Final fullscreen composition pass pipeline
	rasterizationState.cullMode = VK_CULL_MODE_NONE;
	shaderStages[0] = VulkanRenderer::LoadShader(m_device, "Shaders/bin/deferredlighting.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = VulkanRenderer::LoadShader(m_device, "Shaders/bin/deferredlighting.frag.spv",VK_SHADER_STAGE_FRAGMENT_BIT);
	// Empty vertex input state, vertices are generated by the vertex shader
	VkPipelineVertexInputStateCreateInfo emptyInputState = oGFX::vk::inits::pipelineVertexInputStateCreateInfo();
	pipelineCI.pVertexInputState = &emptyInputState;
	pipelineCI.renderPass = VulkanRenderer::defaultRenderPass;
	pipelineCI.layout = compositionPipeLayout;
	colorBlendState = oGFX::vk::inits::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
	blendAttachmentState= oGFX::vk::inits::pipelineColorBlendAttachmentState(0xf, VK_FALSE);

	VK_CHK(vkCreateGraphicsPipelines(m_device.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &compositionPipe));
	VK_NAME(m_device.logicalDevice, "compositionPipe", compositionPipe);

	vkDestroyShaderModule(m_device.logicalDevice,shaderStages[0].module , nullptr);
	vkDestroyShaderModule(m_device.logicalDevice, shaderStages[1].module, nullptr);
}
