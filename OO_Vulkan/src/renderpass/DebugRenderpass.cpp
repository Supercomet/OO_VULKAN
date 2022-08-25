#include "DebugRenderpass.h"

#include <array>
#include <typeindex>

#include "Window.h"
#include "VulkanRenderer.h"
#include "VulkanUtils.h"

#include "../shaders/shared_structs.h"
#include "MathCommon.h"

DECLARE_RENDERPASS(DebugRenderpass);

void DebugRenderpass::Init()
{
	CreatePushconstants();
	CreateDebugRenderpass();
	CreatePipeline();
	InitDebugBuffers();
}

void DebugRenderpass::Draw()
{
	auto& vr = *VulkanRenderer::get();

	auto swapchainIdx = vr.swapchainIdx;
	auto* windowPtr = vr.windowPtr;
	auto& commandBuffers = vr.commandBuffers;

	std::array<VkClearValue, 2> clearValues{};
	//clearValues[0].color = { 0.6f,0.65f,0.4f,1.0f };
	clearValues[0].color = { 0.1f,0.1f,0.1f,1.0f };
	clearValues[1].depthStencil.depth = {1.0f };
	//Information about how to begin a render pass (only needed for graphical applications)
	VkRenderPassBeginInfo renderPassBeginInfo = oGFX::vkutils::inits::renderPassBeginInfo();
	renderPassBeginInfo.renderPass = debugRenderpass;									//render pass to begin
	renderPassBeginInfo.renderArea.offset = { 0,0 };								//start point of render pass in pixels
	renderPassBeginInfo.renderArea.extent = vr.m_swapchain.swapChainExtent;			//size of region to run render pass on (Starting from offset)
	renderPassBeginInfo.pClearValues = clearValues.data();							//list of clear values
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size()); // no clearing

	renderPassBeginInfo.framebuffer =  vr.swapChainFramebuffers[swapchainIdx];
	
	const VkCommandBuffer cmdlist = vr.commandBuffers[swapchainIdx];
    PROFILE_GPU_CONTEXT(cmdlist);
    PROFILE_GPU_EVENT("DebugRender");

	vkCmdBeginRenderPass(cmdlist, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	SetDefaultViewportAndScissor(cmdlist);

	vkCmdBindPipeline(cmdlist, VK_PIPELINE_BIND_POINT_GRAPHICS, debugDrawLinesPSO);

	uint32_t dynamicOffset = 0;
	std::array<VkDescriptorSet, 3> descriptorSetGroup =
	{ 
		vr.descriptorSet_gpuscene,  
		vr.descriptorSets_uniform[swapchainIdx],
		vr.descriptorSet_bindless 
	};
	vkCmdBindDescriptorSets(cmdlist,VK_PIPELINE_BIND_POINT_GRAPHICS,  vr.indirectPSOLayout,
		0, static_cast<uint32_t>(descriptorSetGroup.size()), descriptorSetGroup.data(), 1, &dynamicOffset);

	glm::mat4 xform{ 1.0f };
	vkCmdPushConstants(cmdlist,
		vr.indirectPSOLayout,
		VK_SHADER_STAGE_ALL,    	// stage to push constants to
		0,							// offset of push constants to update
		sizeof(glm::mat4),			// size of data being pushed
		glm::value_ptr(xform));		// actualy data being pushed (could be an array));

	VkDeviceSize offsets[] = { 0 };	

	// just draw the whole set of debug stuff
	vkCmdBindIndexBuffer(cmdlist,  vr.g_debugDrawIndxBuffer.getBuffer(),0, VK_INDEX_TYPE_UINT32);
	vkCmdBindVertexBuffers(cmdlist, VERTEX_BUFFER_ID, 1,vr.g_debugDrawVertBuffer.getBufferPtr(), offsets);
	vkCmdDrawIndexed(cmdlist, static_cast<uint32_t>(vr.g_debugDrawIndxBuffer.size()) , 1, 0, 0, 0);

	for (size_t i = 0; i < vr.debugDrawBufferCnt; i++)
	{
		if (vr.g_b_drawDebug[i])
		{
			auto& debug = vr.g_DebugDraws[i];
			vkCmdBindIndexBuffer(cmdlist,  debug.ibo.getBuffer(),0, VK_INDEX_TYPE_UINT32);
			vkCmdBindVertexBuffers(cmdlist, VERTEX_BUFFER_ID, 1, debug.vbo.getBufferPtr(), offsets);
			vkCmdDrawIndexed(cmdlist, static_cast<uint32_t>(debug.indices.size()) , 1, 0, 0, 0);
		}
	}

	vkCmdEndRenderPass(cmdlist);
}

void DebugRenderpass::Shutdown()
{
	VulkanRenderer* vr = VulkanRenderer::get();
	auto& device = vr->m_device.logicalDevice;

	vkDestroyRenderPass(device, debugRenderpass, nullptr);
	vkDestroyPipeline(device, debugDrawLinesPSO, nullptr);
	vr->g_debugDrawVertBuffer.destroy();
	vr->g_debugDrawIndxBuffer.destroy();
}

void DebugRenderpass::CreateDebugRenderpass()
{
	auto& vr = *VulkanRenderer::get();
	VkAttachmentDescription colourAttachment = {};
	colourAttachment.format = vr.m_swapchain.swapChainImageFormat;  //format to use for attachment
	colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;//number of samples to use for multisampling
	colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;//descripts what to do with attachment before rendering
	colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;//describes what to do with attachment after rendering
	colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; //describes what do with with stencil before rendering
	colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; //describes what do with with stencil before rendering

																		//frame buffer data will be stored as image, but images can be given different data layouts
																		//to give optimal use for certain operations
	colourAttachment.initialLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL; //image data layout before render pass starts
																		 //colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; //image data layout aftet render pass ( to change to)
	colourAttachment.finalLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL; //image data layout aftet render pass ( to change to)
	
	// If editor??
	//colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; //image data layout aftet render pass ( to change to)

																	   // Depth attachment of render pass
	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = oGFX::ChooseSupportedFormat(vr.m_device,
		{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// REFERENCES 
	//Attachment reference uses an atttachment index that refers to index i nthe attachment list passed to renderPassCreataeInfo
	VkAttachmentReference  colourAttachmentReference = {};
	colourAttachmentReference.attachment = 0;
	colourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Depth attachment reference
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

	//conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	// Transiotion msut happen after...
	subpassDependancies[0].srcSubpass = VK_SUBPASS_EXTERNAL; //subpass index (VK_SUBPASS_EXTERNAL = special vallue meaning outside of renderpass)
	subpassDependancies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // Pipeline stage
	subpassDependancies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT; //Stage acces mas (memory access)
																	  // but must happen before...
	subpassDependancies[0].dstSubpass = 0;
	subpassDependancies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependancies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependancies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;


	//conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	// Transiotion msut happen after...
	subpassDependancies[1].srcSubpass = 0;
	subpassDependancies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependancies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	// but must happen before...
	subpassDependancies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependancies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependancies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependancies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	std::array<VkAttachmentDescription, 2> renderpassAttachments = { colourAttachment,depthAttachment };

	//create info for render pass
	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(renderpassAttachments.size());
	renderPassCreateInfo.pAttachments = renderpassAttachments.data();
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependancies.size());
	renderPassCreateInfo.pDependencies = subpassDependancies.data();


	VK_CHK( vkCreateRenderPass(vr.m_device.logicalDevice, &renderPassCreateInfo, nullptr, &debugRenderpass));
	VK_NAME(vr.m_device.logicalDevice, "debugRenderpass", debugRenderpass);

}

void DebugRenderpass::CreatePipeline()
{
	auto& vr = *VulkanRenderer::get();
	using namespace oGFX;

	auto& bindingDescription = GetGFXVertexInputBindings();
	auto& attributeDescriptions= GetGFXVertexInputAttributes();
	auto& m_device = vr.m_device;
	
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = oGFX::vkutils::inits::pipelineVertexInputStateCreateInfo(bindingDescription,attributeDescriptions);
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = oGFX::vkutils::inits::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0 ,VK_FALSE);
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = oGFX::vkutils::inits::pipelineViewportStateCreateInfo(1,1,0);
	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = oGFX::vkutils::inits::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL,VK_CULL_MODE_BACK_BIT,VK_FRONT_FACE_COUNTER_CLOCKWISE,0);
	VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = oGFX::vkutils::inits::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT,0);

	std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = oGFX::vkutils::inits::pipelineDynamicStateCreateInfo(dynamicStateEnables);
	
	VkPipelineColorBlendAttachmentState colourState = oGFX::vkutils::inits::pipelineColorBlendAttachmentState(0x0000000F,VK_TRUE);
	colourState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colourState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colourState.colorBlendOp = VK_BLEND_OP_ADD;

	colourState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colourState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colourState.alphaBlendOp = VK_BLEND_OP_ADD;
	VkPipelineColorBlendStateCreateInfo colourBlendingCreateInfo = oGFX::vkutils::inits::pipelineColorBlendStateCreateInfo(1,&colourState);

	// -- PIPELINE LAYOUT 
	std::array<VkDescriptorSetLayout, 3> descriptorSetLayouts = 
	{
		LayoutDB::gpuscene, // (set = 0)
		LayoutDB::uniform, // (set = 1)
		LayoutDB::bindless // (set = 2)
	};

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = 
		oGFX::vkutils::inits::pipelineLayoutCreateInfo(descriptorSetLayouts.data(),static_cast<uint32_t>(descriptorSetLayouts.size()));
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

	// indirect pipeline
	//VK_CHK(vkCreatePipelineLayout(m_device.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &vr.indirectPipeLayout));
	//VK_NAME(vr.m_device.logicalDevice, "indirectPipeLayout", indirectPipeLayout);
	// go back to normal pipelines

	// Create Pipeline Layout
	//result = vkCreatePipelineLayout(m_device.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
	//if (result != VK_SUCCESS)
	//{
	//	throw std::runtime_error("Failed to create Pipeline Layout!");
	//}
	std::array<VkPipelineShaderStageCreateInfo,2>shaderStages = {};
	
	// -- DEPTH STENCIL TESTING --	
	VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = oGFX::vkutils::inits::pipelineDepthStencilStateCreateInfo(VK_TRUE,VK_TRUE, VK_COMPARE_OP_LESS);

																	// -- GRAPHICS PIPELINE CREATION --
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = oGFX::vkutils::inits::pipelineCreateInfo(vr.indirectPSOLayout,vr.renderPass_default);
	pipelineCreateInfo.stageCount = 2;								//number of shader stages
	pipelineCreateInfo.pStages = shaderStages.data();				//list of sader stages
	pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;	//all the fixed funciton pipeline states
	pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
	pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
	pipelineCreateInfo.pColorBlendState = &colourBlendingCreateInfo;
	pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;

	// we use less for normal pipeline
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputCreateInfo.vertexAttributeDescriptionCount = 5;

	shaderStages[0] = vr.LoadShader(m_device,"Shaders/bin/shader.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = vr.LoadShader(m_device,"Shaders/bin/shader.frag.spv",VK_SHADER_STAGE_FRAGMENT_BIT);
	
	rasterizerCreateInfo.polygonMode = VkPolygonMode::VK_POLYGON_MODE_LINE;
	inputAssembly.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	rasterizerCreateInfo.lineWidth = 1.0f;
	//rasterizerCreateInfo.cullMode = VK_CULL_MODE_NONE;
	pipelineCreateInfo.renderPass = debugRenderpass;
	depthStencilCreateInfo.depthWriteEnable = VK_FALSE;
	//depthStencilCreateInfo.depthTestEnable = VK_FALSE;
	VK_CHK(vkCreateGraphicsPipelines(m_device.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &debugDrawLinesPSO));
	VK_NAME(m_device.logicalDevice, "DebugDrawLinesPSO", debugDrawLinesPSO);

	//destroy shader modules after pipeline is created
	vkDestroyShaderModule(m_device.logicalDevice, shaderStages[0].module, nullptr);
	vkDestroyShaderModule(m_device.logicalDevice, shaderStages[1].module, nullptr);
}

void DebugRenderpass::CreatePushconstants()
{
	auto& vr = *VulkanRenderer::get();
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; //shader stage push constant will go to
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(VulkanRenderer::PushConstData);
}

void DebugRenderpass::InitDebugBuffers()
{
}
