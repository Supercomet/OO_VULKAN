#define NOMINMAX
#include "VulkanRenderer.h"
#include <vector>
#include <set>
#include <stdexcept>
#include <array>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/gtc/matrix_transform.hpp"

#include <iostream>

#include <vulkan/vulkan.h>

#pragma warning( push )
#pragma warning( disable : 26451 ) // vendor overflow
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#pragma warning( pop )

#include "VulkanUtils.h"
#include "Window.h"

#include <chrono>
#include <random>

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>


VulkanRenderer::~VulkanRenderer()
{ 
	//wait until no actions being run on device before destorying
	vkDeviceWaitIdle(m_device.logicalDevice);

	DestroyImGUI();

	for (size_t i = 0; i < models.size(); i++)
	{
		models.clear();
	}	
	for (size_t i = 0; i < textureImages.size(); i++)
	{
		vkDestroyImageView(m_device.logicalDevice, textureImageViews[i], nullptr);
		vkDestroyImage(m_device.logicalDevice, textureImages[i], nullptr);
		vkFreeMemory(m_device.logicalDevice, textureImageMemory[i], nullptr);
	}
	for (size_t i = 0; i < newTextures.size(); i++)
	{
		newTextures[i].destroy();
	}

	instanceBuffer.destroy();
	indirectCommandsBuffer.destroy();

	vkDestroyDescriptorPool(m_device.logicalDevice, samplerDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(m_device.logicalDevice, samplerSetLayout, nullptr);

	vkDestroyImageView(m_device.logicalDevice, depthBufferImageView, nullptr);
	vkDestroyImage(m_device.logicalDevice, depthBufferImage, nullptr);
	vkFreeMemory(m_device.logicalDevice, depthBufferImageMemory, nullptr);

	for (auto framebuffer : swapChainFramebuffers)
	{
		vkDestroyFramebuffer(m_device.logicalDevice, framebuffer, nullptr);
	}

	vkDestroyDescriptorPool(m_device.logicalDevice, descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(m_device.logicalDevice, descriptorSetLayout, nullptr);
	for (size_t i = 0; i < m_swapchain.swapChainImages.size(); i++)
	{
		vkDestroyBuffer(m_device.logicalDevice, vpUniformBuffer[i], nullptr);
		vkFreeMemory(m_device.logicalDevice, vpUniformBufferMemory[i], nullptr);
	}

	vkDestroySampler(m_device.logicalDevice, textureSampler, nullptr);

	for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
	{
		vkDestroyFence(m_device.logicalDevice, drawFences[i], nullptr);
		vkDestroySemaphore(m_device.logicalDevice, renderFinished[i], nullptr);
		vkDestroySemaphore(m_device.logicalDevice, imageAvailable[i], nullptr);
	}

	vkDestroyPipeline(m_device.logicalDevice, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_device.logicalDevice, pipelineLayout, nullptr);

	vkDestroyPipeline(m_device.logicalDevice, indirectPipeline, nullptr);
	vkDestroyPipelineLayout(m_device.logicalDevice, indirectPipeLayout, nullptr);

	//vkDestroyDescriptorPool(mainDevice.logicalDevice, descriptorPool, nullptr);
	
	if (renderPass)
	{
		vkDestroyRenderPass(m_device.logicalDevice, renderPass, nullptr);
		renderPass = VK_NULL_HANDLE;
	}
}

void VulkanRenderer::Init(const oGFX::SetupInfo& setupSpecs, Window& window)
{
	try
	{	
		CreateInstance(setupSpecs);
		CreateSurface(window);
		AcquirePhysicalDevice();
		CreateLogicalDevice();
		SetupSwapchain();
		CreateRenderpass();
		CreateDescriptorSetLayout();
		CreatePushConstantRange();
		CreateGraphicsPipeline();
		CreateDepthBufferImage();
		CreateFramebuffers();
		CreateCommandBuffers();
		CreateTextureSampler();
		CreateUniformBuffers();
		CreateDescriptorPool();
		CreateDescriptorSets();
		CreateSynchronisation();

		//InitImGUI();
	}
	catch (...)
	{
		throw;
	}
}

void VulkanRenderer::CreateInstance(const oGFX::SetupInfo& setupSpecs)
{
	try
	{
		m_instance.Init(setupSpecs);
	}
	catch(...){
		throw;
	}
}

void VulkanRenderer::CreateSurface(Window& window)
{
    windowPtr = &window;
    m_instance.CreateSurface(window);
}

void VulkanRenderer::AcquirePhysicalDevice()
{
    m_device.InitPhysicalDevice(m_instance);
}

void VulkanRenderer::CreateLogicalDevice()
{
    m_device.InitLogicalDevice(m_instance);
}

void VulkanRenderer::SetupSwapchain()
{
	m_swapchain.Init(m_instance,m_device);
}

void VulkanRenderer::CreateRenderpass()
{
	if (renderPass)
	{
		vkDestroyRenderPass(m_device.logicalDevice, renderPass, nullptr);
	}

	// ATTACHMENTS
	// Colour attachment of render pass
	VkAttachmentDescription colourAttachment = {};
	colourAttachment.format = m_swapchain.swapChainImageFormat;  //format to use for attachment
	colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;//number of samples to use for multisampling
	colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;//descripts what to do with attachment before rendering
	colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;//describes what to do with attachment after rendering
	colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; //describes what do with with stencil before rendering
	colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; //describes what do with with stencil before rendering

																		//frame buffer data will be stored as image, but images can be given different data layouts
																		//to give optimal use for certain operations
	colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; //image data layout before render pass starts
	colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; //image data layout aftet render pass ( to change to)

																	// Depth attachment of render pass
	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = oGFX::ChooseSupportedFormat(m_device,
		{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
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

	VkResult result = vkCreateRenderPass(m_device.logicalDevice, &renderPassCreateInfo, nullptr, &renderPass);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Render Pass");
	}
}

void VulkanRenderer::CreateDescriptorSetLayout()
{
	//UNIFORM VALUES DESCRIPTOR SET LAYOUT
	// UboViewProejction binding info
	VkDescriptorSetLayoutBinding vpLayoutBinding = 
		oGFX::vk::inits::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);

	std::vector<VkDescriptorSetLayoutBinding> layoutBindings = { vpLayoutBinding/*, modelLayoutBinding*/ };

	// Create Descriptor Set Layout with given bindings
	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = 
		oGFX::vk::inits::descriptorSetLayoutCreateInfo(layoutBindings.data(),static_cast<uint32_t>(layoutBindings.size()));		

																					// Create descriptor set layout
	VkResult result = vkCreateDescriptorSetLayout(m_device.logicalDevice, &layoutCreateInfo, nullptr, &descriptorSetLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a descriptor set layout!");
	}

	// CREATE TEXTURE SAMPLER DESCRIPTOR SET LAYOUT
	// Texture binding info
	VkDescriptorSetLayoutBinding samplerLayoutBinding = 
		oGFX::vk::inits::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT,0,MAX_OBJECTS);
	
	// create a descriptor set layout with given bindings for texture
	VkDescriptorSetLayoutCreateInfo textureLayoutCreateInfo = 
		oGFX::vk::inits::descriptorSetLayoutCreateInfo(&samplerLayoutBinding,1);


	VkDescriptorBindingFlags flags[3];
	//flags[0] = 0;
	//flags[1] = 0;
	flags[0] = 	VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

	VkDescriptorSetLayoutBindingFlagsCreateInfo flaginfo{};
	flaginfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	flaginfo.pBindingFlags = flags;
	flaginfo.bindingCount = 1;

	textureLayoutCreateInfo.pNext = &flaginfo;


	result = vkCreateDescriptorSetLayout(m_device.logicalDevice, &textureLayoutCreateInfo, nullptr, &samplerSetLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a descriptor set layout!");
	}

}

void VulkanRenderer::CreatePushConstantRange()
{
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; //shader stage push constant will go to
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(LightData);
}

void VulkanRenderer::CreateGraphicsPipeline()
{
	using namespace oGFX;
	//create pipeline

	//how the data for a single vertex (including infos such as pos, colour, texture, coords, normals etc) is as a whole
	std::vector<VkVertexInputBindingDescription> bindingDescription {	
		oGFX::vk::inits::vertexInputBindingDescription(VERTEX_BUFFER_ID,sizeof(Vertex),VK_VERTEX_INPUT_RATE_VERTEX),

		oGFX::vk::inits::vertexInputBindingDescription(INSTANCE_BUFFER_ID,sizeof(oGFX::InstanceData),VK_VERTEX_INPUT_RATE_INSTANCE),
	};

	//how the data for an attirbute is define in the vertex
	std::vector<VkVertexInputAttributeDescription>attributeDescriptions{
		oGFX::vk::inits::vertexInputAttributeDescription(VERTEX_BUFFER_ID,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex, pos)), //Position attribute
		oGFX::vk::inits::vertexInputAttributeDescription(VERTEX_BUFFER_ID,1,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex, norm)),//normals attribute
		oGFX::vk::inits::vertexInputAttributeDescription(VERTEX_BUFFER_ID,2,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex, tangent)),//tangent attribute
		oGFX::vk::inits::vertexInputAttributeDescription(VERTEX_BUFFER_ID,3,VK_FORMAT_R32G32_SFLOAT	  ,offsetof(Vertex, tex)),    //Texture attribute
		
		// instance data attributes
		oGFX::vk::inits::vertexInputAttributeDescription(INSTANCE_BUFFER_ID,4,VK_FORMAT_R32G32B32A32_SFLOAT	,offsetof(oGFX::InstanceData, matrix)+sizeof(float) *0),//Position attribute
		oGFX::vk::inits::vertexInputAttributeDescription(INSTANCE_BUFFER_ID,5,VK_FORMAT_R32G32B32A32_SFLOAT	,offsetof(oGFX::InstanceData, matrix)+sizeof(float) *4), //colour attribute
		oGFX::vk::inits::vertexInputAttributeDescription(INSTANCE_BUFFER_ID,6,VK_FORMAT_R32G32B32A32_SFLOAT	,offsetof(oGFX::InstanceData, matrix)+sizeof(float) *8),////scale attribute
		oGFX::vk::inits::vertexInputAttributeDescription(INSTANCE_BUFFER_ID,7,VK_FORMAT_R32G32B32A32_SFLOAT	,offsetof(oGFX::InstanceData, matrix)+sizeof(float) *12),////scale attribute
		oGFX::vk::inits::vertexInputAttributeDescription(INSTANCE_BUFFER_ID,8,VK_FORMAT_R32_SINT			,offsetof(oGFX::InstanceData, albedo)), // albedo attribute
		oGFX::vk::inits::vertexInputAttributeDescription(INSTANCE_BUFFER_ID,9,VK_FORMAT_R32_SINT			,offsetof(oGFX::InstanceData, normal)), // normal attribute
		oGFX::vk::inits::vertexInputAttributeDescription(INSTANCE_BUFFER_ID,10,VK_FORMAT_R32_SINT			,offsetof(oGFX::InstanceData, occlusion)), // normal attribute
		oGFX::vk::inits::vertexInputAttributeDescription(INSTANCE_BUFFER_ID,11,VK_FORMAT_R32_SINT			,offsetof(oGFX::InstanceData, roughness)), // normal attribute
	};


	// -- VERTEX INPUT -- 
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = oGFX::vk::inits::pipelineVertexInputStateCreateInfo(bindingDescription,attributeDescriptions);

	// __ INPUT ASSEMBLY __
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = oGFX::vk::inits::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0 ,VK_FALSE);


	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = oGFX::vk::inits::pipelineViewportStateCreateInfo(1,1,0);

	//Dynami states to enable	
	std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = oGFX::vk::inits::pipelineDynamicStateCreateInfo(dynamicStateEnables);

	// -- RASTERIZER --
	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = oGFX::vk::inits::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL,VK_CULL_MODE_BACK_BIT,VK_FRONT_FACE_COUNTER_CLOCKWISE,0);
	
	// -- MULTI SAMPLING --
	VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = oGFX::vk::inits::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT,0);

	//-- BLENDING --
	//Blending decies how to blend a new color being written to a a fragment with the old value
	//blend attachment state (how blending is handled
	
	VkPipelineColorBlendAttachmentState colourState = oGFX::vk::inits::pipelineColorBlendAttachmentState(0x0000000F,VK_TRUE);

	//blending uses equation : (srcColourBlendFactor * new colour ) colourBlendOp ( dstColourBlendFActor*old colour)
	colourState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colourState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colourState.colorBlendOp = VK_BLEND_OP_ADD;

	//summarsed : (VK_BLEND_FACTOR_SRC_ALPHA * new colour) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old colour)
	//			  (new colour alpha * new colour) + ((1 - new colour alpha) * old colour)

	colourState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colourState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colourState.alphaBlendOp = VK_BLEND_OP_ADD;
	//summarised : (1 * new alpha) + (0 * old alpha) = new alpha


	VkPipelineColorBlendStateCreateInfo colourBlendingCreateInfo = oGFX::vk::inits::pipelineColorBlendStateCreateInfo(1,&colourState);

	// -- PIPELINE LAYOUT 
	std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = { descriptorSetLayout, samplerSetLayout };

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =  
		oGFX::vk::inits::pipelineLayoutCreateInfo(descriptorSetLayouts.data(),static_cast<uint32_t>(descriptorSetLayouts.size()));
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

	// indirect pipeline
	VkResult result = vkCreatePipelineLayout(m_device.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &indirectPipeLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Pipeline Layout!");
	}
	// go back to normal pipelines

	// Create Pipeline Layout
	result = vkCreatePipelineLayout(m_device.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Pipeline Layout!");
	}
	std::array<VkPipelineShaderStageCreateInfo,2>shaderStages = {};
	
	// -- DEPTH STENCIL TESTING --	
	VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = oGFX::vk::inits::pipelineDepthStencilStateCreateInfo(VK_TRUE,VK_TRUE, VK_COMPARE_OP_LESS);

																	// -- GRAPHICS PIPELINE CREATION --
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = oGFX::vk::inits::pipelineCreateInfo(pipelineLayout,renderPass);
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


	//graphics pipeline creation requires array of shader stages create

	//create graphics pipeline
	shaderStages[0]  = LoadShader("Shaders/indirect.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = LoadShader("Shaders/indirect.frag.spv",VK_SHADER_STAGE_FRAGMENT_BIT);

	pipelineCreateInfo.layout = indirectPipeLayout;
	// Indirect pipeline
	result = vkCreateGraphicsPipelines(m_device.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &indirectPipeline);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Graphics Pipeline!");
	}
	//destroy indirect shader modules 
	vkDestroyShaderModule(m_device.logicalDevice, shaderStages[0].module, nullptr);
	vkDestroyShaderModule(m_device.logicalDevice, shaderStages[1].module, nullptr);


	pipelineCreateInfo.layout = pipelineLayout;
	// we use less for normal pipeline
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputCreateInfo.vertexAttributeDescriptionCount = 4;

	shaderStages[0] = LoadShader("Shaders/shader.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = LoadShader("Shaders/shader.frag.spv",VK_SHADER_STAGE_FRAGMENT_BIT);

	result = vkCreateGraphicsPipelines(m_device.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Graphics Pipeline!");
	}

	//destroy shader modules after pipeline is created
	vkDestroyShaderModule(m_device.logicalDevice, shaderStages[0].module, nullptr);
	vkDestroyShaderModule(m_device.logicalDevice, shaderStages[1].module, nullptr);

}

void VulkanRenderer::CreateDepthBufferImage()
{
	if (depthBufferImage)
	{
		vkDestroyImageView(m_device.logicalDevice, depthBufferImageView,nullptr);
		vkDestroyImage(m_device.logicalDevice, depthBufferImage,nullptr);
		vkFreeMemory(m_device.logicalDevice, depthBufferImageMemory, nullptr);
	}
	using namespace oGFX;
	//get supported format for depth buffer
	m_swapchain.depthFormat = ChooseSupportedFormat(m_device,
		{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

	//create depth buffer image
	depthBufferImage = CreateImage(m_device,m_swapchain.swapChainExtent.width,m_swapchain.swapChainExtent.height, m_swapchain.depthFormat,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depthBufferImageMemory);

	//create depth buffer image view
	depthBufferImageView = CreateImageView(m_device,depthBufferImage, m_swapchain.depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void VulkanRenderer::CreateFramebuffers()
{
	for (size_t i = 0; i < swapChainFramebuffers.size(); i++)
	{
		vkDestroyFramebuffer(m_device.logicalDevice, swapChainFramebuffers[i], nullptr);
	}

	//resize framebuffer count to equal swapchain image count
	swapChainFramebuffers.resize(m_swapchain.swapChainImages.size());

	//create a frame buffer for each swapchain image
	for (size_t i = 0; i < swapChainFramebuffers.size(); i++)
	{
		std::array<VkImageView, 2> attachments = {
			m_swapchain.swapChainImages[i].imageView,
			depthBufferImageView
		};

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = renderPass;										//render pass layout the frame buffer will be used with
		framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferCreateInfo.pAttachments = attachments.data();							//list of attachments (1:1 with render pass)
		framebufferCreateInfo.width = m_swapchain.swapChainExtent.width;
		framebufferCreateInfo.height = m_swapchain.swapChainExtent.height;
		framebufferCreateInfo.layers = 1;

		VkResult result = vkCreateFramebuffer(m_device.logicalDevice, &framebufferCreateInfo, nullptr, &swapChainFramebuffers[i]);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a Framebuffer!");
		}
	}
}

void VulkanRenderer::CreateCommandBuffers()
{
	// resize command buffers count to have one for each frame buffer
	commandBuffers.resize(swapChainFramebuffers.size());

	VkCommandBufferAllocateInfo cbAllocInfo = {};
	cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cbAllocInfo.commandPool = m_device.commandPool;
	cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// VK_COMMAND_BUFFER_LEVEL_PRIMARY : buffer you submit directly to queue, cant be called  by other buffers
															//VK_COMMAND_BUFFER_LEVEL_SECONDARY :  buffer cant be called directly, can be called from other buffers via "vkCmdExecuteCommands" when recording commands in primary buffer
	cbAllocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	//allocate command buffers and place handles in array of buffers
	VkResult result = vkAllocateCommandBuffers(m_device.logicalDevice, &cbAllocInfo, commandBuffers.data());
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Command Buffers!");
	}
}

void VulkanRenderer::CreateTextureSampler()
{
	// Sampler creation info
	VkSamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;							// how to render when image is magnified on screen
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;							// how to render when image is minified on the screen
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;		// how to handle texture wrap in U direction
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;		// how to handle texture wrap in V direction
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;		// how to handle texture wrap in W direction
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;		// border beyond texture ( only works for border clamp )
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;					// Whether coords should be normalized (between 0 and 1)
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;			// Mipmap interpolation mode
	samplerCreateInfo.mipLodBias = 0.0f;									// Level of details bias for mip level
	samplerCreateInfo.minLod = 0.0f;										// minimum level of detail to pick mip level
	samplerCreateInfo.maxLod = 0.0f;										// maximum level of detail to pick mip level
	samplerCreateInfo.anisotropyEnable = VK_TRUE;							// Enable anisotropy
	samplerCreateInfo.maxAnisotropy = 16;									// Anisotropy sample level

	VkResult result = vkCreateSampler(m_device.logicalDevice, &samplerCreateInfo, nullptr, &textureSampler);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a texture sampler!");
	}
}

void VulkanRenderer::CreateSynchronisation()
{
	imageAvailable.resize(MAX_FRAME_DRAWS);
	renderFinished.resize(MAX_FRAME_DRAWS);
	drawFences.resize(MAX_FRAME_DRAWS);
	//Semaphore creation information
	VkSemaphoreCreateInfo semaphorecreateInfo = {};
	semaphorecreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	//fence creating information
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
	{
		if (vkCreateSemaphore(m_device.logicalDevice, &semaphorecreateInfo, nullptr, &imageAvailable[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_device.logicalDevice, &semaphorecreateInfo, nullptr, &renderFinished[i]) != VK_SUCCESS ||
			vkCreateFence(m_device.logicalDevice, &fenceCreateInfo, nullptr,&drawFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a Semaphore and/or Fence!");
		}
	}
}

void VulkanRenderer::CreateUniformBuffers()
{	
	// ViewProjection buffer size
	VkDeviceSize vpBufferSize = sizeof(UboViewProjection);

	//// LightData bufffer size
	//VkDeviceSize modelBufferSize = modelUniformAlignment * MAX_OBJECTS;

	// One uniform buffer for each image (and by extension, command buffer)
	vpUniformBuffer.resize(m_swapchain.swapChainImages.size());
	vpUniformBufferMemory.resize(m_swapchain.swapChainImages.size());
	//modelDUniformBuffer.resize(swapChainImages.size());
	//modelDUniformBufferMemory.resize(swapChainImages.size());

	//create uniform buffers
	for (size_t i = 0; i < m_swapchain.swapChainImages.size(); i++)
	{
		oGFX::CreateBuffer(m_device.physicalDevice, m_device.logicalDevice, vpBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vpUniformBuffer[i], &vpUniformBufferMemory[i]);
		/*createBuffer(mainDevice.physicalDevice, mainDevice.logicalDevice, modelBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &modelDUniformBuffer[i], &modelDUniformBufferMemory[i]);*/
	}
}

void VulkanRenderer::CreateDescriptorPool()
{
	// CREATE UNIFORM DESCRIPTOR POOL
	//descriptor is an individual piece of data // it is NOT a descriptor SET
	// Type of descriptors + how many DESCRIPTORS, not DESCRIPTOR_SETS (combined makes the pool size)
	// ViewProjection pool
	VkDescriptorPoolSize vpPoolsize = oGFX::vk::inits::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, vpUniformBuffer.size());

	//// LightData pool (DYNAMIC)
	//VkDescriptorPoolSize modelPoolSize{};
	//modelPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	//modelPoolSize.descriptorCount = static_cast<uint32_t>(modelDUniformBuffer.size());

	//list of pool sizes
	std::vector<VkDescriptorPoolSize> descriptorPoolSizes = { vpPoolsize /*, modelPoolSize*/ };

	//data to create the descriptor pool
	VkDescriptorPoolCreateInfo poolCreateInfo = oGFX::vk::inits::descriptorPoolCreateInfo(descriptorPoolSizes,static_cast<uint32_t>(m_swapchain.swapChainImages.size()));
	//create descriptor pool
	VkResult result = vkCreateDescriptorPool(m_device.logicalDevice, &poolCreateInfo, nullptr, &descriptorPool);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a descriptor pool!");
	}

	// Create Sampler Descriptor pool
	// Texture sampler pool
	VkDescriptorPoolSize samplerPoolSize = oGFX::vk::inits::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_OBJECTS);
	std::vector<VkDescriptorPoolSize> samplerpoolSizes = { samplerPoolSize };

	VkDescriptorPoolCreateInfo samplerPoolCreateInfo = oGFX::vk::inits::descriptorPoolCreateInfo(samplerpoolSizes,MAX_OBJECTS);
	result = vkCreateDescriptorPool(m_device.logicalDevice, &samplerPoolCreateInfo, nullptr, &samplerDescriptorPool);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a descriptor pool!");
	}

	// Variable descriptor
	VkDescriptorSetVariableDescriptorCountAllocateInfoEXT variableDescriptorCountAllocInfo = {};

	uint32_t variableDescCounts[] = { MAX_OBJECTS };
	variableDescriptorCountAllocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
	variableDescriptorCountAllocInfo.descriptorSetCount = 1;
	variableDescriptorCountAllocInfo.pDescriptorCounts  = variableDescCounts;

	//Descriptor set allocation info
	VkDescriptorSetAllocateInfo setAllocInfo = oGFX::vk::inits::descriptorSetAllocateInfo(samplerDescriptorPool,&samplerSetLayout,1);
	setAllocInfo.pNext = &variableDescriptorCountAllocInfo;

	//Allocate our descriptor sets
	result = vkAllocateDescriptorSets(m_device.logicalDevice, &setAllocInfo, &globalSamplers);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("FAiled to allocate texture descriptor sets!");
	}

}

void VulkanRenderer::CreateDescriptorSets()
{
	// Resize Descript Set List so one for every buffer
	descriptorSets.resize(m_swapchain.swapChainImages.size());

	std::vector<VkDescriptorSetLayout> setLayouts(m_swapchain.swapChainImages.size(),descriptorSetLayout);

	// Descriptor set allocation info
	VkDescriptorSetAllocateInfo setAllocInfo{};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = descriptorPool;									// Pool to allocate descriptor set from
	setAllocInfo.descriptorSetCount = static_cast<uint32_t>(m_swapchain.swapChainImages.size()); // Number of sets to alllocate
	setAllocInfo.pSetLayouts = setLayouts.data();									// Layouts to use to allocate sets (1:1 relationship)

																					//Allocate descriptor sets
	VkResult result = vkAllocateDescriptorSets(m_device.logicalDevice, &setAllocInfo, descriptorSets.data());
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate descriptor sets!");
	}

	//update all of descriptor set bindings
	for (size_t i = 0; i < m_swapchain.swapChainImages.size(); i++)
	{
		// VIEW PROJECTION DESCRIPTOR
		// Buffer info and data offset info
		VkDescriptorBufferInfo vpBufferInfo{};
		vpBufferInfo.buffer = vpUniformBuffer[i];	// buffer to get data from
		vpBufferInfo.offset = 0;					// position of start of data
		vpBufferInfo.range = sizeof(UboViewProjection);			// size of data

																// Data about connection between binding and buffer
		VkWriteDescriptorSet vpSetWrite{};
		vpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vpSetWrite.dstSet = descriptorSets[i];							// Descriptor set to update
		vpSetWrite.dstBinding = 0;										// Binding to update (matches with binding on layout/shader)
		vpSetWrite.dstArrayElement = 0;								// index in array to update
		vpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Type of descriptor
		vpSetWrite.descriptorCount = 1;								// amount to update
		vpSetWrite.pBufferInfo = &vpBufferInfo;						// information about the buffer data to bind

		//// MODEL DESCRIPTOR
		//// model buffer binding info
		//VkDescriptorBufferInfo modelBufferInfo{};
		//modelBufferInfo.buffer = modelDUniformBuffer[i];
		//modelBufferInfo.offset = 0;
		//modelBufferInfo.range = modelUniformAlignment;
		
		//VkWriteDescriptorSet modelSetWrite{};
		//modelSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		//modelSetWrite.dstSet = descriptorSets[i];
		//modelSetWrite.dstBinding = 1;
		//modelSetWrite.dstArrayElement = 0;
		//modelSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		//modelSetWrite.descriptorCount = 1;
		//modelSetWrite.pBufferInfo = &modelBufferInfo;

																	// List of Descriptor set writes
		std::vector<VkWriteDescriptorSet> setWrites = { vpSetWrite  /*, modelSetWrite*/ };

		// Update the descriptor sets with new buffer/binding info
		vkUpdateDescriptorSets(m_device.logicalDevice, static_cast<uint32_t>(setWrites.size()), setWrites.data(),
			0, nullptr);
	}
}

void VulkanRenderer::InitImGUI()
{
	VkAttachmentDescription attachment = {};
	attachment.format = m_swapchain.swapChainImageFormat;
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // Draw GUI on what exitst
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// TODO: make sure we set the previous renderpass to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL 
	// since this will be the final pass (before presentation) instead
	attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // final layout for presentation.

	VkAttachmentReference color_attachment = {};
	color_attachment.attachment = 0;
	color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // create dependancy outside current renderpass
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // make sure pixels have been fully rendered before performing this pass
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // same thing
	dependency.srcAccessMask = 0;  // or VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.attachmentCount = 1;
	info.pAttachments = &attachment;
	info.subpassCount = 1;
	info.pSubpasses = &subpass;
	info.dependencyCount = 1;
	info.pDependencies = &dependency;
	if (vkCreateRenderPass(m_device.logicalDevice, &info, nullptr, &m_imguiConfig.renderPass) != VK_SUCCESS) {
		throw std::runtime_error("Could not create Dear ImGui's render pass");
	}

	std::vector<VkDescriptorPoolSize> pool_sizes
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo dpci = oGFX::vk::inits::descriptorPoolCreateInfo(pool_sizes,1);
	vkCreateDescriptorPool(m_device.logicalDevice, &dpci, nullptr, &m_imguiConfig.descriptorPools);

	// Setup Dear ImGui context
	//IMGUI_CHECKVERSION();
	//ImGui::CreateContext();
	//ImGuiIO& io = ImGui::GetIO();
	////io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	//
	//// Setup Dear ImGui style
	//ImGui::StyleColorsDark();
	//
	////ImGui_ImplGlfw_InitForVulkan(window, true);
	//ImGui_ImplVulkan_InitInfo init_info = {};
	//init_info.Instance = m_instance.instance;
	//init_info.PhysicalDevice = m_device.physicalDevice;
	//init_info.Device = m_device.logicalDevice;
	//init_info.QueueFamily = m_device.queueIndices.graphicsFamily;
	//init_info.Queue = m_device.graphicsQueue;
	//init_info.PipelineCache = VK_NULL_HANDLE;
	//init_info.DescriptorPool = m_imguiConfig.descriptorPools;
	//init_info.Allocator = nullptr;
	//init_info.MinImageCount = m_swapchain.minImageCount;
	//init_info.ImageCount = m_swapchain.swapChainImages.size();
	//init_info.CheckVkResultFn = VK_NULL_HANDLE; // can be used to handle the error checking
	//ImGui_ImplVulkan_Init(&init_info, m_imguiConfig.renderPass);


}

void VulkanRenderer::DestroyImGUI()
{
	vkDestroyRenderPass(m_device.logicalDevice, m_imguiConfig.renderPass, nullptr);
	vkDestroyDescriptorPool(m_device.logicalDevice, m_imguiConfig.descriptorPools, nullptr);
}

void VulkanRenderer::UpdateIndirectCommands()
{
	indirectCommands.clear();
	// Create on indirect command for node in the scene with a mesh attached to it
	uint32_t m = 0;
	for (auto& model :models)
	{
		for (auto& node: model.nodes)
		{
			for(auto& child : node->children)
			{
				if (child->meshes.size())
				{
					VkDrawIndexedIndirectCommand indirectCmd{};
					indirectCmd.instanceCount = OBJECT_INSTANCE_COUNT;
					indirectCmd.firstInstance = m * OBJECT_INSTANCE_COUNT;
					// @todo: Multiple primitives
					// A glTF node may consist of multiple primitives, so we may have to do multiple commands per mesh
					indirectCmd.firstIndex = child->meshes[0]->indicesOffset;
					indirectCmd.indexCount = child->meshes[0]->indicesCount;

					indirectCmd.vertexOffset = child->meshes[0]->vertexOffset;

					// for counting
					//vertexCount += node->meshes[0]->vertexCount;
					indirectCommands.push_back(indirectCmd);

					m++;
				}
			}
			if (node->meshes.size())
			{
				VkDrawIndexedIndirectCommand indirectCmd{};
				indirectCmd.instanceCount = OBJECT_INSTANCE_COUNT;
				indirectCmd.firstInstance = m * OBJECT_INSTANCE_COUNT;
				// @todo: Multiple primitives
				// A glTF node may consist of multiple primitives, so we may have to do multiple commands per mesh
				indirectCmd.firstIndex = node->meshes[0]->indicesOffset;
				indirectCmd.indexCount = node->meshes[0]->indicesCount;

				indirectCmd.vertexOffset = node->meshes[0]->vertexOffset;

				// for counting
				//vertexCount += node->meshes[0]->vertexCount;
				indirectCommands.push_back(indirectCmd);

				m++;
			}
		}
	}

	//std::cout << "Triangles rendered : " << models[0].indices.count * OBJECT_INSTANCE_COUNT /3 << std::endl;
	indirectDrawCount = static_cast<uint32_t>(indirectCommands.size());

	objectCount = 0;
	for (auto indirectCmd : indirectCommands)
	{
		objectCount += indirectCmd.instanceCount;
	}
	//vertexCount *= objectCount /3;

	vk::Buffer stagingBuffer;	
	m_device.CreateBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer,
		indirectCommands.size() * sizeof(VkDrawIndexedIndirectCommand),
		indirectCommands.data());

	if (indirectCommandsBuffer.size == 0)
	{
	m_device.CreateBuffer(
		VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&indirectCommandsBuffer,
		stagingBuffer.size);
	}
	m_device.CopyBuffer(&stagingBuffer, &indirectCommandsBuffer, m_device.graphicsQueue);

	stagingBuffer.destroy();

}

void VulkanRenderer::UpdateInstanceData()
{
	if (instanceBuffer.size != 0) return;
	
	using namespace std::chrono;
	uint64_t curr = duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
	std::default_random_engine rndEngine(curr);
	std::uniform_real_distribution<float> uniformDist(0.0f, 1.0f);
	std::vector<oGFX::InstanceData> instanceData;
	instanceData.resize(objectCount);

	constexpr float radius = 1000.0f;
	constexpr float offset = 2000.0f;

	{

	glm::mat4 matrix = glm::mat4(1.0f); 
	float scale = 0.01;
	matrix = glm::scale(matrix, glm::vec3(scale));
	matrix = glm::translate(matrix, glm::vec3(0.0f));
	instanceData[0].matrix = matrix; 
	instanceData[0].albedo = models[0].textures.albedo;
	instanceData[0].normal = models[0].textures.normal;
	instanceData[0].occlusion = models[0].textures.occlusion;
	instanceData[0].roughness = models[0].textures.roughness;
	}

	for (uint32_t i = 1; i < objectCount; i++) {
		float theta = 2 * float(glm::pi<float>()) * uniformDist(rndEngine);
		float phi = acos(1 - 2 * uniformDist(rndEngine));
		float scale = 0.001f + uniformDist(rndEngine) * 0.005f;
		
		glm::vec3 pos = glm::vec3(sin(phi) * cos(theta), 0.0f, cos(phi)) * radius;
		pos += glm::normalize(pos-glm::vec3(0.0f)) * offset;

		glm::mat4 matrix = glm::mat4(1.0f); 
		matrix = glm::scale(matrix, glm::vec3(scale));
		matrix = glm::rotate(matrix, theta, glm::vec3(0.0f,1.0f,0.0f));
		matrix = glm::translate(matrix, pos);

		instanceData[i].matrix = matrix; 

		auto val = (1) / OBJECT_INSTANCE_COUNT;
		val = 1;
		if (val == 1)
		{
			instanceData[i].albedo =  1;
		}
		if (val == 2)
		{
			instanceData[i].albedo =  2;
		}
		if (val == 0)
		{
			instanceData[i].albedo =  1;
		}
		
		instanceData[i].albedo = models[0].textures.albedo + uniformDist(rndEngine) * 100;
		instanceData[i].normal = models[0].textures.normal;
		instanceData[i].occlusion = models[0].textures.occlusion;
		instanceData[i].roughness = models[0].textures.roughness;
		//instanceData[i].albedo = uniformDist(rndEngine) * 4;
		//instanceData[i].albedo = models[0].nodes[0]->meshes[0]->textureIndex;
	}

	vk::Buffer stagingBuffer;
	m_device.CreateBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer,
		instanceData.size() * sizeof(oGFX::InstanceData),
		instanceData.data());

	if (instanceBuffer.size == 0)
	{
		m_device.CreateBuffer(
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&instanceBuffer,
			stagingBuffer.size);
	}

	m_device.CopyBuffer(&stagingBuffer, &instanceBuffer, m_device.graphicsQueue);

	stagingBuffer.destroy();
}

void VulkanRenderer::Draw()
{
	if (resizeSwapchain || windowPtr->m_width == 0 ||windowPtr->m_height ==0)
	{
		if (ResizeSwapchain() == false) return;
		resizeSwapchain = false;
	}

	UpdateIndirectCommands();
	UpdateInstanceData();

	//wait for given fence to signal from last draw before continueing
	vkWaitForFences(m_device.logicalDevice, 1, &drawFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
	//mainually reset fences
	vkResetFences(m_device.logicalDevice, 1, &drawFences[currentFrame]);

	//1. get the next available image to draw to and set somethingg to signal when we're finished with the image ( a semaphere )
	// -- GET NEXT IMAGE
	//get  index of next image to be drawn to , and signal semaphere when ready to be drawn to
	uint32_t imageIndex{};
	VkResult res = vkAcquireNextImageKHR(m_device.logicalDevice, m_swapchain.swapchain, std::numeric_limits<uint64_t>::max(), imageAvailable[currentFrame], VK_NULL_HANDLE, &imageIndex);
	if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR /*|| WINDOW_RESIZED*/)
	{
		resizeSwapchain = true;
	}
	RecordCommands(imageIndex);
	
	camera.updated = true;
	if(camera.updated)
		UpdateUniformBuffers(imageIndex);

	//2. Submit command buffer to queue for execution, make sure it waits for image to be signalled as available before drawing
	//		and signals when it has finished rendering
	// --SUBMIT COMMAND BUFFER TO RENDER
	// Queue submission information
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1; //number of semaphores to wait on
	submitInfo.pWaitSemaphores = &imageAvailable[currentFrame]; //list of semaphores to wait on
	VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};
	submitInfo.pWaitDstStageMask = waitStages; //stages to check semapheres at
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];	// command buffer to submit
	submitInfo.signalSemaphoreCount = 1;						// number of semaphores to signal
	submitInfo.pSignalSemaphores = &renderFinished[currentFrame];				// semphores to signal when command buffer finished

																				//submit command buffer to queue
	VkResult result = vkQueueSubmit(m_device.graphicsQueue, 1, &submitInfo, drawFences[currentFrame]);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to submit command buffer to queue!");
	}

	//3. present image t oscreen when it has signalled finished rendering
	// -- PRESENT RENDERED IMAGE TO SCREEN --
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderFinished[currentFrame];	//semaphores to wait on
	presentInfo.swapchainCount = 1;					//number of swapchains to present to
	presentInfo.pSwapchains = &m_swapchain.swapchain;			//swapchains to present images to
	presentInfo.pImageIndices = &imageIndex;		//index of images in swapchains to present

													//present image
	try
	{
		result = vkQueuePresentKHR(m_device.presentationQueue, &presentInfo);
		if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR /*|| WINDOW_RESIZED*/)
		{
			resizeSwapchain = true;
			return;
		}
		else if(result != VK_SUCCESS && result!= VK_SUBOPTIMAL_KHR)
		{
			std::cout << (int)result;
			throw std::runtime_error("Failed to present image!");
		}
	}
	catch(std::runtime_error e){
		std::cout << e.what();
	}
	//get next frame (use % MAX_FRAME_DRAWS to keep value below max frames)
	currentFrame = (currentFrame + 1) % MAX_FRAME_DRAWS;
}


bool VulkanRenderer::ResizeSwapchain()
{
	while (windowPtr->m_height == 0 || windowPtr->m_width == 0)
	{
		Window::PollEvents();
		if (windowPtr->windowShouldClose) return false;
	}
	m_swapchain.Init(m_instance, m_device);
	CreateRenderpass();
	CreateDepthBufferImage();
	CreateFramebuffers();

	return true;
}

uint32_t VulkanRenderer::LoadMeshFromFile(const std::string& file)
{
	// new model loader
	
	Assimp::Importer importer;
	const aiScene *scene = importer.ReadFile(file,
		  aiProcess_Triangulate                // Make sure we get triangles rather than nvert polygons
		| aiProcess_LimitBoneWeights           // 4 weights for skin model max
		| aiProcess_GenUVCoords                // Convert any type of mapping to uv mapping
		| aiProcess_TransformUVCoords          // preprocess UV transformations (scaling, translation ...)
		| aiProcess_FindInstances              // search for instanced meshes and remove them by references to one master
		| aiProcess_CalcTangentSpace           // calculate tangents and bitangents if possible
		| aiProcess_JoinIdenticalVertices      // join identical vertices/ optimize indexing
		| aiProcess_RemoveRedundantMaterials   // remove redundant materials
		| aiProcess_FindInvalidData            // detect invalid model data, such as invalid normal vectors
		| aiProcess_PreTransformVertices       // TODO: remove for skinning?
		| aiProcess_FlipUVs						// TODO: some mesh need
	);

	if (!scene)
	{
		throw std::runtime_error("Failed to load model! (" + file + ")");
	}	
	auto index = models.size();
	models.emplace_back(std::move(Model()));
	
	auto& model = models[index];
	model.device = &m_device;

	std::vector<std::string> textureNames = MeshContainer::LoadMaterials(scene);
	std::vector<int> matToTex(textureNames.size());
	// Loop over textureNames and create textures for them
	for (size_t i = 0; i < textureNames.size(); i++)
	{
		// if material had no texture, set '0' to indicate no texture, texxture 0 will be reserved fora  default texture
		if (textureNames[i].empty())
		{
			matToTex[i] = 0;
		}
		else
		{
			// otherwise create texture and set value to index of new texture
			matToTex[i] =  CreateTexture(textureNames[i]);
		}
	}

	for (auto& node : model.nodes)
	{
		for (auto& mesh : node->meshes)
		{
			mesh->textureIndex = matToTex[mesh->textureIndex];
		}
	}

	std::vector<oGFX::Vertex> verticeBuffer;
	std::vector<uint32_t> indexBuffer;

	model.loadNode(nullptr, scene, *scene->mRootNode,0,verticeBuffer, indexBuffer);

	model.indices.count = static_cast<uint32_t>(indexBuffer.size());
	model.vertices.count = static_cast<uint32_t>(verticeBuffer.size());
	

	{
		using namespace oGFX;
		//get size of buffer needed for vertices
		VkDeviceSize bufferSize = sizeof(Vertex) * verticeBuffer.size();

		//temporary buffer to stage vertex data before transferring to GPU
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory; 
		//create buffer and allocate memory to it
		CreateBuffer(m_device.physicalDevice,m_device.logicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

		//MAP MEMORY TO VERTEX BUFFER
		void *data = nullptr;												//1. create a pointer to a point in normal memory
		vkMapMemory(m_device.logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);	//2. map the vertex buffer to that point
		memcpy(data, verticeBuffer.data(), (size_t)bufferSize);					//3. copy memory from vertices vector to the point
		vkUnmapMemory(m_device.logicalDevice, stagingBufferMemory);							//4. unmap the vertex buffer memory

																			//create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data (also VERTEX_BUFFER)
																			// buffer memory is to be DEVICE_LOCAL_BIT meaning memory is on the GPU and only accessible by the GPU and not the CPU (host)
		CreateBuffer(m_device.physicalDevice, m_device.logicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &model.vertices.buffer, &model.vertices.memory); // VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT make this buffer local to the GPU

																					  //copy staging buffer to vertex buffer on GPU
		CopyBuffer(m_device.logicalDevice, m_device.graphicsQueue, m_device.commandPool, stagingBuffer, model.vertices.buffer, bufferSize);

		//clean up staging buffer parts
		vkDestroyBuffer(m_device.logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(m_device.logicalDevice, stagingBufferMemory, nullptr);
	}

	//CreateIndexBuffer
	{
		using namespace oGFX;
		//get size of buffer needed for vertices
		VkDeviceSize bufferSize = sizeof(decltype(indexBuffer)::value_type) * indexBuffer.size();

		//temporary buffer to stage vertex data before transferring to GPU
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory; 
		//create buffer and allocate memory to it
		CreateBuffer(m_device.physicalDevice,m_device.logicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

		//MAP MEMORY TO VERTEX BUFFER
		void *data = nullptr;												//1. create a pointer to a point in normal memory
		vkMapMemory(m_device.logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);	//2. map the vertex buffer to that point
		memcpy(data, indexBuffer.data(), (size_t)bufferSize);					//3. copy memory from vertices vector to the point
		vkUnmapMemory(m_device.logicalDevice, stagingBufferMemory);				//4. unmap the vertex buffer memory

		//create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data (also VERTEX_BUFFER)
		// buffer memory is to be DEVICE_LOCAL_BIT meaning memory is on the GPU and only accessible by the GPU and not the CPU (host)
		CreateBuffer(m_device.physicalDevice, m_device.logicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &model.indices.buffer, &model.indices.memory); // VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT make this buffer local to the GPU

		//copy staging buffer to vertex buffer on GPU
		CopyBuffer(m_device.logicalDevice, m_device.graphicsQueue, m_device.commandPool, stagingBuffer, model.indices.buffer, bufferSize);

		//clean up staging buffer parts
		vkDestroyBuffer(m_device.logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(m_device.logicalDevice, stagingBufferMemory, nullptr);
	}

	return static_cast<uint32_t>(index);
}

void VulkanRenderer::SetMeshTextures(uint32_t modelID, uint32_t alb, uint32_t norm, uint32_t occlu, uint32_t rough)
{
	models[modelID].textures = { alb,norm,occlu,rough };
}

uint32_t VulkanRenderer::CreateTexture(uint32_t width, uint32_t height, unsigned char* imgData)
{
	using namespace oGFX;
	FileImageData fileData;
	fileData.w = width;
	fileData.h = height;
	fileData.channels = 4;
	fileData.dataSize = fileData.w * fileData.h * fileData.channels;
	fileData.imgData.resize(fileData.dataSize);

	VkBufferImageCopy copyRegion{};
	copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.imageSubresource.mipLevel = 0;
	copyRegion.imageSubresource.baseArrayLayer = 0;
	copyRegion.imageSubresource.layerCount = 1;
	copyRegion.bufferOffset = 0;
	copyRegion.imageExtent.width = fileData.w;
	copyRegion.imageExtent.height = fileData.h;
	copyRegion.imageExtent.depth = 1;
	fileData.mipInformation.push_back(copyRegion);

	memcpy(fileData.imgData.data(), imgData, fileData.dataSize);
	//fileData.imgData = imgData;

	auto ind = CreateTextureImage(fileData);

	//VkImageView imageView = CreateImageView(m_device,textureImages[ind], VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
	//textureImageViews.push_back(imageView);

	//create texture descriptor
	 int descriptorLoc = CreateTextureDescriptor(newTextures[ind]);

	//return location of set with texture
	return descriptorLoc;

}

uint32_t VulkanRenderer::CreateTexture(const std::string& file)
{
	// Create texture image and get its location in array
	uint32_t textureImageLoc = CreateTextureImage(file);

	// Create image view and add to list
	//VkImageView imageView = oGFX::CreateImageView(m_device,textureImages[textureImageLoc], VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
	//textureImageViews.push_back(imageView);

	//create texture descriptor
	int descriptorLoc = CreateTextureDescriptor(newTextures[textureImageLoc]);

	//return location of set with texture
	return descriptorLoc;

}


void VulkanRenderer::RecordCommands(uint32_t currentImage)
{
	//Information about how to begin each command buffer
	VkCommandBufferBeginInfo bufferBeginInfo = oGFX::vk::inits::commandBufferBeginInfo();
	
	std::array<VkClearValue, 2> clearValues{};
	//clearValues[0].color = { 0.6f,0.65f,0.4f,1.0f };
	clearValues[0].color = { 0.1f,0.1f,0.1f,1.0f };
	clearValues[1].depthStencil.depth = {1.0f };

	//Information about how to begin a render pass (only needed for graphical applications)
	VkRenderPassBeginInfo renderPassBeginInfo = oGFX::vk::inits::renderPassBeginInfo();
	renderPassBeginInfo.renderPass = renderPass;									//render pass to begin
	renderPassBeginInfo.renderArea.offset = { 0,0 };								//start point of render pass in pixels
	renderPassBeginInfo.renderArea.extent = m_swapchain.swapChainExtent;			//size of region to run render pass on (Starting from offset)
	renderPassBeginInfo.pClearValues = clearValues.data();							//list of clear values
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());


	renderPassBeginInfo.framebuffer = swapChainFramebuffers[currentImage];

	//start recording commanders to command buffer!
	VkResult result = vkBeginCommandBuffer(commandBuffers[currentImage], &bufferBeginInfo);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to start recording a Command Buffer!");
	}

	//Begin Render Pass
	vkCmdBeginRenderPass(commandBuffers[currentImage], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	//Bind pipeline to be used in render pass
	//vkCmdBindPipeline(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	VkViewport viewport = { 0, float(windowPtr->m_height), float(windowPtr->m_width), -float(windowPtr->m_height), 0, 1 };
	VkRect2D scissor = { {0, 0}, {uint32_t(windowPtr->m_width),uint32_t(windowPtr->m_height) } };
	vkCmdSetViewport(commandBuffers[currentImage], 0, 1, &viewport);
	vkCmdSetScissor(commandBuffers[currentImage], 0, 1, &scissor);

	//for (size_t j = 0; j < modelList.size(); ++j)
	//{
	//	//if (j == 0) continue;
	//
	//	MeshContainer thisModel = modelList[j];
	//	
	//	// Push constants tot shader stage directly, (no buffer)
	//	vkCmdPushConstants(commandBuffers[currentImage],
	//		pipelineLayout,
	//		VK_SHADER_STAGE_VERTEX_BIT,	// stage to push constants to
	//		0,							// offset of push constants to update
	//		sizeof(LightData),			// size of data being pushed
	//		&thisModel.getModel());		// actualy data being pushed (could be an array)
	//	
	//	for (size_t k = 0; k < thisModel.getMeshCount(); k++)
	//	{
	//		VkBuffer vertexBuffers[] = { thisModel.getMesh(k)->getVertexBuffer() };					// buffers to bind
	//		VkDeviceSize offsets[] = { 0 };															//offsets into buffers being bound
	//		vkCmdBindVertexBuffers(commandBuffers[currentImage], 0, 1, vertexBuffers, offsets);		//command to bind vertex buffer before drawing with them
	//	
	//																								//bind mesh index buffer with 0 offset and using the uint32 type
	//		vkCmdBindIndexBuffer(commandBuffers[currentImage], thisModel.getMesh(k)->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
	//	
	//		// Dyanimc offset amount 
	//		//uint32_t dynamicOffset = static_cast<uint32_t>(modelUniformAlignment) * j;
	//	
	//	
	//		std::array<VkDescriptorSet, 2> descriptorSetGroup = { descriptorSets[currentImage],
	//			samplerDescriptorSets[thisModel.getMesh(k)->getTexId()] };
	//	
	//		// bind descriptor sets
	//		vkCmdBindDescriptorSets(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
	//			0, static_cast<uint32_t>(descriptorSetGroup.size()), descriptorSetGroup.data(), 0 /*1*/, nullptr /*&dynamicOffset*/);
	//	
	//		//Execute pipeline
	//		vkCmdDrawIndexed(commandBuffers[currentImage], thisModel.getMesh(k)->getIndexCount(), 1, 0, 0, 0);
	//	}
	//}


	std::array<VkDescriptorSet, 2> descriptorSetGroup = { descriptorSets[currentImage],
		globalSamplers };
	//auto& model = models[0];
	VkDeviceSize offsets[] = { 0 };	
	//
	//vkCmdPushConstants(commandBuffers[currentImage],
	//	pipelineLayout,
	//	VK_SHADER_STAGE_VERTEX_BIT,	// stage to push constants to
	//	0,							// offset of push constants to update
	//	sizeof(LightData),				// size of data being pushed
	//	&modelList[0].getModel());		// actualy data being pushed (could be an array)
	//
	//vkCmdBindVertexBuffers(commandBuffers[currentImage], 0, 1, &model.vertices.buffer, offsets);	
	//vkCmdBindIndexBuffer(commandBuffers[currentImage], model.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
	//vkCmdDrawIndexed(commandBuffers[currentImage], model.indices.count, 1, 0, 0, 0);

	
	 vkCmdBindPipeline(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, indirectPipeline);
	 //std::cout << "Light pos [" << light.position.x << ", " << light.position.y << ", " << light.position.z<<"] ";
	 //std::cout << "\tCamera pos [" << camera.position.x << ", " << camera.position.y << ", " << camera.position.z<<"]\n";
	 
	 vkCmdPushConstants(commandBuffers[currentImage],
		 indirectPipeLayout,
		 VK_SHADER_STAGE_VERTEX_BIT,	// stage to push constants to
		 0,							// offset of push constants to update
		 sizeof(LightData),			// size of data being pushed
		 &light);		// actualy data being pushed (could be an array));
	
	vkCmdBindDescriptorSets(commandBuffers[currentImage],VK_PIPELINE_BIND_POINT_GRAPHICS,indirectPipeLayout,
		0, static_cast<uint32_t>(descriptorSetGroup.size()), descriptorSetGroup.data(), 0 /*1*/, nullptr /*&dynamicOffset*/);
	 vkCmdBindVertexBuffers(commandBuffers[currentImage], VERTEX_BUFFER_ID, 1, &models[0].vertices.buffer, offsets);

	 // Binding point 1 : Instance data buffer
	 vkCmdBindVertexBuffers(commandBuffers[currentImage], INSTANCE_BUFFER_ID, 1, &instanceBuffer.buffer, offsets);

	 vkCmdBindIndexBuffer(commandBuffers[currentImage], models[0].indices.buffer, 0, VK_INDEX_TYPE_UINT32);
	 if (m_device.enabledFeatures.multiDrawIndirect)
	 {
		 vkCmdDrawIndexedIndirect(commandBuffers[currentImage], indirectCommandsBuffer.buffer, 0, indirectDrawCount, sizeof(VkDrawIndexedIndirectCommand));
	 }
	 else
	 {
		for (size_t i = 0; i < indirectCommands.size(); i++)
		{		
			vkCmdDrawIndexedIndirect(commandBuffers[currentImage], indirectCommandsBuffer.buffer, i * sizeof(VkDrawIndexedIndirectCommand), 1, sizeof(VkDrawIndexedIndirectCommand));
		}
	 }

	// End Render  Pass
	vkCmdEndRenderPass(commandBuffers[currentImage]);

	//stop recording to command buffer
	result = vkEndCommandBuffer(commandBuffers[currentImage]);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to stop recording a Command Buffer!");
	}

}

void VulkanRenderer::UpdateUniformBuffers(uint32_t imageIndex)
{		

	float height = static_cast<float>(windowPtr->m_height);
	float width = static_cast<float>(windowPtr->m_width);
	float ar = width / height;

	//uboViewProjection.view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -1.0f));
	//uboViewProjection.projection = glm::ortho(-ar, ar, -1.0f, 1.0f, -100.f, 100.0f);
	//uboViewProjection.projection = glm::perspective(45.f,ar,0.01f,100.0f);

	uboViewProjection.projection = camera.matrices.perspective;
	uboViewProjection.view = camera.matrices.view;
	uboViewProjection.cameraPos = glm::vec4(camera.position,1.0);
	

	//uboViewProjection.projection = oGFX::customOrtho(1.0,10.0f,-1.0f,10.0f);
	//uboViewProjection.projection[1][1] *= -1.0f;

	//copy VP data
	void *data;
	vkMapMemory(m_device.logicalDevice, vpUniformBufferMemory[imageIndex], 0, sizeof(UboViewProjection), 0, &data);
	memcpy(data, &uboViewProjection, sizeof(UboViewProjection));
	vkUnmapMemory(m_device.logicalDevice, vpUniformBufferMemory[imageIndex]);

	////copy LightData data
	//for (size_t i = 0; i < meshList.size(); ++i)
	//{
	//	LightData *thisModel = (LightData *)((uint64_t)modelTransferSpace + (i * modelUniformAlignment));
	//	*thisModel = meshList[i].getModel();
	//}
	//vkMapMemory(mainDevice.logicalDevice, modelDUniformBufferMemory[imageIndex], 0, modelUniformAlignment*meshList.size(), 0, &data);
	//memcpy(data, modelTransferSpace, modelUniformAlignment * meshList.size());
	//vkUnmapMemory(mainDevice.logicalDevice, modelDUniformBufferMemory[imageIndex]);

}

uint32_t VulkanRenderer::CreateTextureImage(const std::string& fileName)
{
	//Load image file
	oGFX::FileImageData imageData;
	imageData.Create(fileName);
	
	//int width{}, height{};
	//VkDeviceSize imageSize;
	//unsigned char *imageData = oGFX::LoadTextureFromFile(fileName, width, height, imageSize);

	auto value = CreateTextureImage(imageData);

	imageData.Free();
	return value;
}

uint32_t VulkanRenderer::CreateTextureImage(const oGFX::FileImageData& imageInfo)
{
	VkDeviceSize imageSize = imageInfo.dataSize;

	auto indx = newTextures.size();
	newTextures.push_back(vk::Texture2D());

	newTextures[indx].fromBuffer((void*)imageInfo.imgData.data(), imageSize, imageInfo.format, imageInfo.w, imageInfo.h,imageInfo.mipInformation, &m_device, m_device.graphicsQueue);

	// Return index of new texture image
	return indx;
}

VkPipelineShaderStageCreateInfo VulkanRenderer::LoadShader(const std::string& fileName, VkShaderStageFlagBits stage)
{
	// SHADER STAGE CREATION INFORMATION
	VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
	shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	//shader stage name
	shaderStageCreateInfo.stage = stage;

	//build shader modules to link to pipeline
	//read in SPIR-V code of shaders
	auto shaderCode = oGFX::readFile(fileName);
	VkShaderModule shaderModule = oGFX::CreateShaderModule(m_device,shaderCode);

	//shader module to be used by stage
	shaderStageCreateInfo.module = shaderModule;
	//pointer to the shader starting function
	shaderStageCreateInfo.pName = "main";


	assert(shaderStageCreateInfo.module != VK_NULL_HANDLE);
	return shaderStageCreateInfo;
}

uint32_t VulkanRenderer:: CreateTextureDescriptor(vk::Texture2D texture)
{
	
	std::vector<VkWriteDescriptorSet> writeSets{
		oGFX::vk::inits::writeDescriptorSet(globalSamplers, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &texture.descriptor),
	};

	auto index = static_cast<uint32_t>(samplerDescriptorSets.size());
	samplerDescriptorSets.push_back(globalSamplers);
	writeSets[0].dstArrayElement = index;

	vkUpdateDescriptorSets(m_device.logicalDevice, writeSets.size(), writeSets.data(), 0, nullptr);

	return index;

}
