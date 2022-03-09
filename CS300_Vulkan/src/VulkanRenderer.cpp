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


VulkanRenderer::~VulkanRenderer()
{ 
	//wait until no actions being run on device before destorying
	vkDeviceWaitIdle(m_device.logicalDevice);

	for (size_t i = 0; i < modelList.size(); i++)
	{
		modelList[i].destroyMeshModel();
	}	
	for (size_t i = 0; i < textureImages.size(); i++)
	{
		vkDestroyImageView(m_device.logicalDevice, textureImageViews[i], nullptr);
		vkDestroyImage(m_device.logicalDevice, textureImages[i], nullptr);
		vkFreeMemory(m_device.logicalDevice, textureImageMemory[i], nullptr);
	}

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

	vkDestroyCommandPool(m_device.logicalDevice, graphicsCommandPool, nullptr);
	vkDestroyPipeline(m_device.logicalDevice, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_device.logicalDevice, pipelineLayout, nullptr);

	//vkDestroyDescriptorPool(mainDevice.logicalDevice, descriptorPool, nullptr);
	
	if (renderPass)
	{
		vkDestroyRenderPass(m_device.logicalDevice, renderPass, nullptr);
		renderPass = VK_NULL_HANDLE;
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
	subpassDependancies[0].dependencyFlags = 0;


	//conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	// Transiotion msut happen after...
	subpassDependancies[1].srcSubpass = 0;
	subpassDependancies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependancies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	// but must happen before...
	subpassDependancies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependancies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependancies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependancies[1].dependencyFlags = 0;

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
	VkDescriptorSetLayoutBinding vpLayoutBinding{};
	vpLayoutBinding.binding = 0;											// Binding point in shader (designated by binding number in shader)
	vpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;		// type of descriptor ( uniform, dynamic uniform, image sampler, etc)
	vpLayoutBinding.descriptorCount = 1;									// Number of descriptors for binding
	vpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;				// Shader stage to bind to
	vpLayoutBinding.pImmutableSamplers = nullptr;							// For texture : can make sampler immutable by specifiying in layout

																			//// Model binding info
																			//VkDescriptorSetLayoutBinding modelLayoutBinding{};
																			//modelLayoutBinding.binding = 1;
																			//modelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
																			//modelLayoutBinding.descriptorCount = 1;
																			//modelLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
																			//modelLayoutBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> layoutBindings = { vpLayoutBinding/*, modelLayoutBinding*/ };

	// Create Descriptor Set Layout with given bindings
	VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());	// number of binding infos
	layoutCreateInfo.pBindings = layoutBindings.data();								// array of binding infos

																					// Create descriptor set layout
	VkResult result = vkCreateDescriptorSetLayout(m_device.logicalDevice, &layoutCreateInfo, nullptr, &descriptorSetLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a descriptor set layout!");
	}

	// CREATE TEXTURE SAMPLER DESCRIPTOR SET LAYOUT
	// Texture binding info
	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 0;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	// create a descriptor set layout with given bindings for texture
	VkDescriptorSetLayoutCreateInfo textureLayoutCreateInfo{};
	textureLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	textureLayoutCreateInfo.bindingCount = 1;
	textureLayoutCreateInfo.pBindings = &samplerLayoutBinding;

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
	pushConstantRange.size = sizeof(Model);
}

void VulkanRenderer::CreateGraphicsPipeline()
{
	using namespace oGFX;
	//read in SPIR-V code of shaders
	auto vertexShaderCode = readFile("Shaders/shader.vert.spv");
	auto fragmentShaderCode = readFile("Shaders/shader.frag.spv");

	//build shader modules to link to graphics pipeline
	VkShaderModule vertexShaderModule = CreateShaderModule(m_device,vertexShaderCode);
	VkShaderModule fragmentShaderModule = CreateShaderModule(m_device,fragmentShaderCode);

	// SHADER STAGE CREATION INFORMATION

	// VERTEX STAGE CREATION INFORMATION
	VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
	vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	//shader stage name
	vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	//shader module to be used by stage
	vertexShaderCreateInfo.module = vertexShaderModule;
	//pointer to the shader starting function
	vertexShaderCreateInfo.pName = "main";

	// FRAGMENT STAGE CREATION INFORMATION
	VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
	fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	//shader stage name
	fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	//shader module to be used by stage
	fragmentShaderCreateInfo.module = fragmentShaderModule;
	//pointer to the shader starting function
	fragmentShaderCreateInfo.pName = "main";

	//put shader stage creation into array
	//graphics pipeline creation requires array of shader stages create
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

	//create pipeline

	//how the data for a single vertex (including infos such as pos, colour, texture, coords, normals etc) is as a whole
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0; //can bind multiple streams of data, this defines which one
	bindingDescription.stride = sizeof(Vertex); //size of a single vertex object in memory
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; //how to move between data after each vertex
																//VK_VERTEX_INPUT_RATE_INSTANCE : move to a vertex for the next instance

																//how the data for an attirbute is define in the vertex
	std::array<VkVertexInputAttributeDescription, 3>attributeDescriptions;

	//Position attribute
	attributeDescriptions[0].binding = 0; //which binding the data is at  ( should be same as above)
	attributeDescriptions[0].location = 0; //location in shader where data will be read from
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // format the data will take (also helps define the size of the data) 
	attributeDescriptions[0].offset = offsetof(Vertex, pos); // where this attribute is defined in the data for a single vertex

															 //colour attribute
	attributeDescriptions[1].binding = 0; 
	attributeDescriptions[1].location = 1; 
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;  
	attributeDescriptions[1].offset = offsetof(Vertex, col); 

	//Texture attribute
	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, tex);

	// -- VERTEX INPUT -- 
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription; //list of vertext binding descriptions (data spacing / stride info)
	vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); // list of vertext attribute descriptions (data format and wheret to bind to/from)

																					   // __ INPUT ASSEMBLY __
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // primitive type to assemble vertices as
																  //allow overriding of strip topology to start new primitives
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// -- VIEWPORT & SCISSOR --
	// Createa  viewport info struct
	VkViewport viewport = {};
	viewport.x = 0.0f;									//x start coord
	viewport.y = 0.0f;									//y start coord
	viewport.width = (float)m_swapchain.swapChainExtent.width;		// viewport width
	viewport.height = (float)m_swapchain.swapChainExtent.height;	// viewport height
	viewport.minDepth = 0.0f;							//min frame buffer depth
	viewport.maxDepth = 1.0f;							//max frame buffer depth

														// create a sciossor info struct
	VkRect2D scissor = {};
	scissor.offset = { 0,0 }; //offset to use region from
	scissor.extent = m_swapchain.swapChainExtent; //extent to describe region to use, starting at offset

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	// -- DYNAMIC STATES -- (TODO: )
	//Dynami states to enable
	std::vector<VkDynamicState> dynamicStateEnables;
	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT); //dynamic viewport : can resize in command buffer with vkCmdSetViewport(commandbuffer,0,1,&viewport);
	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR); //dynamic viewport : can resize in command buffer with vkCmdSetScissor(commandbuffer,0,1,&scissor);

															 //// Dyanimc state creation info
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
	dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();

	// -- RASTERIZER --
	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
	rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCreateInfo.depthClampEnable = VK_FALSE;					// change if fragments beyond/near the far planes are clipped(default) or clamped.. must enable depth clamp in device features
	rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;			// wheter to desicard data and skip rasterizer. never creates fragments, only suitable for pipeline without framebuffer output.
	rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;			// how to handle filling points between vertices
	rasterizerCreateInfo.lineWidth = 1.0f;								// how thick lines should be when drawn
	rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;				// which face of the triangle to cull (backface cull)
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;	// Winding to determine which side is front
	rasterizerCreateInfo.depthBiasEnable = VK_FALSE;					// Wheter to add debt biast to fragments. (good for stoppging "shadow acne" in shadow mapping)


																		// -- MULTI SAMPLING --
	VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {};
	multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingCreateInfo.sampleShadingEnable = VK_FALSE; //enable multi sample shading or not;
	multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; //number of samples to use per fragment;

																		  //-- BLENDING --
																		  //Blending decies how to blend a new color being written to a a fragment with the old value

																		  //blend attachment state (how blending is handled_
	VkPipelineColorBlendAttachmentState colourState = {};
	colourState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
		| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;//colors to apply blending to
	colourState.blendEnable = VK_TRUE; //enable blending

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


	VkPipelineColorBlendStateCreateInfo colourBlendingCreateInfo = {};
	colourBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colourBlendingCreateInfo.logicOpEnable = VK_FALSE; // alternative to calculations is to use logical operations
	colourBlendingCreateInfo.attachmentCount = 1;
	colourBlendingCreateInfo.pAttachments = &colourState;

	// -- PIPELINE LAYOUT 
	std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = { descriptorSetLayout, samplerSetLayout };

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

	// Create Pipeline Layout
	VkResult result = vkCreatePipelineLayout(m_device.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Pipeline Layout!");
	}

	// -- DEPTH STENCIL TESTING --	
	VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo{};
	depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilCreateInfo.depthTestEnable = VK_TRUE;				//enable checking depth to determine fragment write
	depthStencilCreateInfo.depthWriteEnable = VK_TRUE;				// enable writing to depth buffer (to replace old values)
	depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;		// comparison operation that allows an overwrite (is in front)
	depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;		// Depth bounds test: does the depth value exist between 2 bounds
	depthStencilCreateInfo.stencilTestEnable = VK_FALSE;			// Enable stencil test

																	// -- GRAPHICS PIPELINE CREATION --
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount = 2;								//number of shader stages
	pipelineCreateInfo.pStages = shaderStages;						//list of sader stages
	pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;	//all the fixed funciton pipeline states
	pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
	pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
	pipelineCreateInfo.pColorBlendState = &colourBlendingCreateInfo;
	pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
	pipelineCreateInfo.layout = pipelineLayout;						//pipeline layout pipeline should use
	pipelineCreateInfo.renderPass = renderPass;						//render pass description the pipeline is compatible with
	pipelineCreateInfo.subpass = 0;									//subpass of rneder pass to use with pipeline

																	//pipeline derivatives : can create multiple pipels that derive from one another for optimization
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; //existing pipeline to derive from
	pipelineCreateInfo.basePipelineIndex = -1; //or index of pipeline being created to derive from (in case creating multiple at once)

											   //create graphics pipeline
	result = vkCreateGraphicsPipelines(m_device.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Graphics Pipeline!");
	}

	//destroy shader modules after pipeline is created
	vkDestroyShaderModule(m_device.logicalDevice, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(m_device.logicalDevice, vertexShaderModule, nullptr);
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

void VulkanRenderer::CreateCommandPool()
{
	using namespace oGFX;
	// Get indicies of queue families from device
	QueueFamilyIndices queueFamilyIndices = GetQueueFamilies(m_device.physicalDevice,m_instance.surface);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily; //Queue family type that buffers from this command pool will use

																   //create a graphics queue family command pool
	VkResult result = vkCreateCommandPool(m_device.logicalDevice, &poolInfo, nullptr, &graphicsCommandPool);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to createa a command pool!");
	}
}

void VulkanRenderer::CreateCommandBuffers()
{
	// resize command buffers count to have one for each frame buffer
	commandBuffers.resize(swapChainFramebuffers.size());

	VkCommandBufferAllocateInfo cbAllocInfo = {};
	cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cbAllocInfo.commandPool = graphicsCommandPool;
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

	//// Model bufffer size
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
	VkDescriptorPoolSize vpPoolsize{};
	vpPoolsize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	vpPoolsize.descriptorCount = static_cast<uint32_t>(vpUniformBuffer.size());

	//// Model pool (DYNAMIC)
	//VkDescriptorPoolSize modelPoolSize{};
	//modelPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	//modelPoolSize.descriptorCount = static_cast<uint32_t>(modelDUniformBuffer.size());

	//list of pool sizes
	std::vector<VkDescriptorPoolSize> descriptorPoolSizes = { vpPoolsize /*, modelPoolSize*/ };

	//data to create the descriptor pool
	VkDescriptorPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.maxSets = static_cast<uint32_t>(m_swapchain.swapChainImages.size()); // Maximum number of descriptor sets that can be created from pool
	poolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());										// Amount of pool sizes being passed
	poolCreateInfo.pPoolSizes = descriptorPoolSizes.data();									// Pool sizes to create pool with

																							//create descriptor pool
	VkResult result = vkCreateDescriptorPool(m_device.logicalDevice, &poolCreateInfo, nullptr, &descriptorPool);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a descriptor pool!");
	}

	// Create Sampler Descriptor pool
	// Texture sampler pool
	VkDescriptorPoolSize samplerPoolSize{};
	samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerPoolSize.descriptorCount = MAX_OBJECTS;

	VkDescriptorPoolCreateInfo samplerPoolCreateInfo{};
	samplerPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	samplerPoolCreateInfo.maxSets = MAX_OBJECTS;
	samplerPoolCreateInfo.poolSizeCount = 1;
	samplerPoolCreateInfo.pPoolSizes = &samplerPoolSize;

	result = vkCreateDescriptorPool(m_device.logicalDevice, &samplerPoolCreateInfo, nullptr, &samplerDescriptorPool);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a descriptor pool!");
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

void VulkanRenderer::Draw()
{
	if (resizeSwapchain || windowPtr->m_width == 0 ||windowPtr->m_height ==0)
	{
		if (ResizeSwapchain() == false) return;
		resizeSwapchain = false;
	}

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

void VulkanRenderer::UpdateModel(int modelId, glm::mat4 newModel)
{
	if(modelId>= modelList.size())return; //error selection!!
	modelList[modelId].setModel(newModel);
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


uint32_t VulkanRenderer::CreateTexture(uint32_t width, uint32_t height, const unsigned char* imgData)
{
	using namespace oGFX;
	auto ind = CreateTextureImage(width, height, imgData);

	VkImageView imageView = CreateImageView(m_device,textureImages[ind], VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
	textureImageViews.push_back(imageView);

	//create texture descriptor
	int descriptorLoc = CreateTextureDescriptor(imageView);

	//return location of set with texture
	return descriptorLoc;

}

uint32_t VulkanRenderer::CreateTexture(const std::string& file)
{
	// Create texture image and get its location in array
	uint32_t textureImageLoc = CreateTextureImage(file);

	// Create image view and add to list
	VkImageView imageView = oGFX::CreateImageView(m_device,textureImages[textureImageLoc], VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
	textureImageViews.push_back(imageView);

	//create texture descriptor
	int descriptorLoc = CreateTextureDescriptor(imageView);

	//return location of set with texture
	return descriptorLoc;

}

uint32_t VulkanRenderer::CreateMeshModel(std::vector<oGFX::Vertex>& vertices,std::vector<uint32_t>& indices)
{
	Mesh mesh(m_device.physicalDevice, m_device.logicalDevice, m_device.graphicsQueue, graphicsCommandPool, &vertices, &indices,0);
	MeshModel model({ mesh });
	modelList.push_back(model);
	return static_cast<uint32_t>(modelList.size() - 1);
}


uint32_t VulkanRenderer::CreateMeshModel(const std::string& file)
{
	Assimp::Importer importer;
	const aiScene *scene = importer.ReadFile(file, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);

	if (!scene)
	{
		throw std::runtime_error("Failed to load model! (" + file + ")");
	}

	//get vector of all materials with 1:1 ID placement
	std::vector<std::string> textureNames = MeshModel::LoadMaterials(scene);

	// Conversion from the materials list IDs  to our Descriptor Array IDs
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

	// load in all our meshes
	std::vector<Mesh> modelMeshes = MeshModel::LoadNode(m_device.physicalDevice, m_device.logicalDevice, 
		 m_device.graphicsQueue, graphicsCommandPool, scene->mRootNode, scene, matToTex);

	MeshModel meshModel = MeshModel(modelMeshes);
	modelList.push_back(meshModel);
	return static_cast<uint32_t>(modelList.size() - 1);

}

void VulkanRenderer::RecordCommands(uint32_t currentImage)
{
	//Information about how to begin each command buffer
	VkCommandBufferBeginInfo bufferBeginInfo = {};
	bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	//using fences its not relevant
	//bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; //buffer can be resubmitted when it has already been submitted and it is waiting for execution

	//Information about how to begin a render pass (only needed for graphical applications)
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = renderPass; //render pass to begin
	renderPassBeginInfo.renderArea.offset = { 0,0 }; //start point of render pass in pixels
	renderPassBeginInfo.renderArea.extent = m_swapchain.swapChainExtent; //size of region to run render pass on (Starting from offset)
	std::array<VkClearValue, 2> clearValues{};

	//clearValues[0].color = { 0.6f,0.65f,0.4f,1.0f };
	clearValues[0].color = { 0.1f,0.1f,0.1f,1.0f };
	clearValues[1].depthStencil.depth = {1.0f };

	renderPassBeginInfo.pClearValues = clearValues.data(); //list of clear values
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
	vkCmdBindPipeline(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	VkViewport viewport = { 0, float(windowPtr->m_height), float(windowPtr->m_width), -float(windowPtr->m_height), 0, 1 };
	VkRect2D scissor = { {0, 0}, {uint32_t(windowPtr->m_width),uint32_t(windowPtr->m_height) } };
	vkCmdSetViewport(commandBuffers[currentImage], 0, 1, &viewport);
	vkCmdSetScissor(commandBuffers[currentImage], 0, 1, &scissor);

	for (size_t j = 0; j < modelList.size(); ++j)
	{
		MeshModel thisModel = modelList[j];
		
		// Push constants tot shader stage directly, (no buffer)
		vkCmdPushConstants(commandBuffers[currentImage],
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT,	// stage to push constants to
			0,							// offset of push constants to update
			sizeof(Model),				// size of data being pushed
			&thisModel.getModel());		// actualy data being pushed (could be an array)
		
		for (size_t k = 0; k < thisModel.getMeshCount(); k++)
		{
			VkBuffer vertexBuffers[] = { thisModel.getMesh(k)->getVertexBuffer() };					// buffers to bind
			VkDeviceSize offsets[] = { 0 };												//offsets into buffers being bound
			vkCmdBindVertexBuffers(commandBuffers[currentImage], 0, 1, vertexBuffers, offsets);		//command to bind vertex buffer before drawing with them
		
																									//bind mesh index buffer with 0 offset and using the uint32 type
			vkCmdBindIndexBuffer(commandBuffers[currentImage], thisModel.getMesh(k)->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
		
			// Dyanimc offset amount 
			//uint32_t dynamicOffset = static_cast<uint32_t>(modelUniformAlignment) * j;
		
		
		
			std::array<VkDescriptorSet, 2> descriptorSetGroup = { descriptorSets[currentImage],
				samplerDescriptorSets[thisModel.getMesh(k)->getTexId()] };
		
			// bind descriptor sets
			vkCmdBindDescriptorSets(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
				0, static_cast<uint32_t>(descriptorSetGroup.size()), descriptorSetGroup.data(), 0 /*1*/, nullptr /*&dynamicOffset*/);
		
			//Execute pipeline
			vkCmdDrawIndexed(commandBuffers[currentImage], thisModel.getMesh(k)->getIndexCount(), 1, 0, 0, 0);
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

	//uboViewProjection.projection = oGFX::customOrtho(1.0,10.0f,-1.0f,10.0f);
	//uboViewProjection.projection[1][1] *= -1.0f;

	//copy VP data
	void *data;
	vkMapMemory(m_device.logicalDevice, vpUniformBufferMemory[imageIndex], 0, sizeof(UboViewProjection), 0, &data);
	memcpy(data, &uboViewProjection, sizeof(UboViewProjection));
	vkUnmapMemory(m_device.logicalDevice, vpUniformBufferMemory[imageIndex]);

	////copy Model data
	//for (size_t i = 0; i < meshList.size(); ++i)
	//{
	//	Model *thisModel = (Model *)((uint64_t)modelTransferSpace + (i * modelUniformAlignment));
	//	*thisModel = meshList[i].getModel();
	//}
	//vkMapMemory(mainDevice.logicalDevice, modelDUniformBufferMemory[imageIndex], 0, modelUniformAlignment*meshList.size(), 0, &data);
	//memcpy(data, modelTransferSpace, modelUniformAlignment * meshList.size());
	//vkUnmapMemory(mainDevice.logicalDevice, modelDUniformBufferMemory[imageIndex]);

}

uint32_t VulkanRenderer::CreateTextureImage(const std::string& fileName)
{
	//Load image file
	int width{}, height{};
	VkDeviceSize imageSize;
	unsigned char *imageData = oGFX::LoadTextureFromFile(fileName, width, height, imageSize);

	return CreateTextureImage(width, height, imageData);
}


uint32_t VulkanRenderer::CreateTextureImage(uint32_t width, uint32_t height, const unsigned char* imgData)
{
	using namespace oGFX;
	//Load image file
	VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * static_cast<VkDeviceSize>(height) * 4;

	// Create staging buffer to hold loaded data, ready to copy to device
	VkBuffer imageStagingBuffer;
	VkDeviceMemory imageStagingBufferMemory;
	CreateBuffer(m_device.physicalDevice, m_device.logicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&imageStagingBuffer, &imageStagingBufferMemory);

	// copy image data to staging buffer
	void *data;
	vkMapMemory(m_device.logicalDevice, imageStagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, imgData, static_cast<size_t>(imageSize));
	vkUnmapMemory(m_device.logicalDevice, imageStagingBufferMemory);

	// Create image to hold final texture
	VkImage texImage;
	VkDeviceMemory texImageMemory;
	texImage = CreateImage(m_device,width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &texImageMemory);

	// Copy data to image
	//transition image to be DST for copy operation
	TransitionImageLayout(m_device.logicalDevice, m_device.graphicsQueue, graphicsCommandPool,
		texImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	// Copy image data
	CopyImageBuffer(m_device.logicalDevice, m_device.graphicsQueue, graphicsCommandPool, imageStagingBuffer, texImage, width, height);

	//transition image to be Shader readable for shader usage
	TransitionImageLayout(m_device.logicalDevice,m_device. graphicsQueue, graphicsCommandPool,
		texImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	//Add texture data to vector for refrence.
	textureImages.push_back(texImage);
	textureImageMemory.push_back(texImageMemory);

	// Destroy staginb uffers
	vkDestroyBuffer(m_device.logicalDevice, imageStagingBuffer, nullptr);
	vkFreeMemory(m_device.logicalDevice, imageStagingBufferMemory, nullptr);

	// Return index of new texture image
	return static_cast<uint32_t>(textureImages.size() - 1);
}

uint32_t VulkanRenderer:: CreateTextureDescriptor(VkImageView textureImage)
{
	VkDescriptorSet descriptorSet;

	// Descriptor set allocation info
	VkDescriptorSetAllocateInfo setAllocInfo{};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = samplerDescriptorPool;
	setAllocInfo.descriptorSetCount = 1;
	setAllocInfo.pSetLayouts = &samplerSetLayout;

	// Allocate our descriptor sets
	VkResult result = vkAllocateDescriptorSets(m_device.logicalDevice, &setAllocInfo, &descriptorSet);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("FAiled to allocate texture descriptor sets!");
	}

	// Texture image info
	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // image outout when in use
	imageInfo.imageView = textureImage;// image to bind to set
	imageInfo.sampler = textureSampler; // sampler to use for set


										//Descriptor write info
	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;

	// Update the new descriptor set
	vkUpdateDescriptorSets(m_device.logicalDevice, 1, &descriptorWrite, 0, nullptr);

	// Add descriptor set to list
	samplerDescriptorSets.push_back(descriptorSet);

	return static_cast<uint32_t>(samplerDescriptorSets.size() - 1);
}
