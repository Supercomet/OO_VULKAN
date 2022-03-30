#pragma once
#include <vulkan/vulkan.h>
#include "glm/glm.hpp"

#include <string>
#include <vector>
struct VulkanInstance;
struct VulkanDevice;
namespace oGFX
{

	glm::mat4 customOrtho(float aspect_ratio, float size, float nr, float fr);
	
	// Indices (locations) of Queue Familities (if they exist)
	struct QueueFamilyIndices
	{
		int graphicsFamily = -1; //location of graphics queue family //as per vulkan standard, if we have a graphics family, we have a transfer family
		int presentationFamily = -1;

		//check if queue familities are valid
		bool isValid()
		{
			return graphicsFamily >= 0 && presentationFamily >=0;
		}
	};


	struct SwapChainDetails
	{
		//surfaces properties , image sizes/extents etc...
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		//surface image formats, eg. RGBA, data size of each color
		std::vector<VkSurfaceFormatKHR> formats;
		//how images should be presentated to screen, filo fifo etc..
		std::vector<VkPresentModeKHR> presentationModes;
		SwapChainDetails() :surfaceCapabilities{} {}
	};

	struct SwapChainImage
	{
		VkImage image;
		VkImageView imageView;
	};

	struct Vertex
	{
		//float pos[3] ; // Vertex position (x, y, z)
		//float col[3] ; // Vertex colour (r, g, b)
		//float tex[2] ; // Texture Coords(u,v)
		glm::vec3 pos; // Vertex position (x, y, z)
		glm::vec3 col; // Vertex colour (r, g, b)
		glm::vec2 tex; // Texture Coords(u,v)
	};

	oGFX::SwapChainDetails GetSwapchainDetails(VulkanInstance& instance,VkPhysicalDevice device);
	oGFX::QueueFamilyIndices GetQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

	VkSurfaceFormatKHR ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR ChooseBestPresentationMode(const std::vector<VkPresentModeKHR> presentationModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);

	VkImageView CreateImageView(VulkanDevice& device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

	VkFormat ChooseSupportedFormat(VulkanDevice& device, const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);

	VkImage CreateImage(VulkanDevice& device,uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory* imageMemory);

	uint32_t FindMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes, VkMemoryPropertyFlags properties);

	VkShaderModule CreateShaderModule(VulkanDevice& device, const std::vector<char>& code);

	std::vector<char> readFile(const std::string& filename);

	void CreateBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize bufferSize,
		VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags bufferProperties,
		VkBuffer* buffer, VkDeviceMemory* bufferMemory);

	void CopyBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool,
		VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize);

	VkCommandBuffer beginCommandBuffer(VkDevice device, VkCommandPool commandPool);

	void endAndSubmitCommandBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer);
	
	void TransitionImageLayout(VkDevice device, VkQueue queue, VkCommandPool commandPool,
		VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

	void CopyImageBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool,
		VkBuffer srcBuffer, VkImage image, uint32_t width, uint32_t height);

	unsigned char* LoadTextureFromFile(const std::string& fileName, int& width, int& height, uint64_t& imageSize);


	namespace vk
	{
		namespace inits
		{

			inline VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(
				VkDescriptorType type,
				VkShaderStageFlags stageFlags,
				uint32_t binding,
				uint32_t descriptorCount = 1)
			{
				VkDescriptorSetLayoutBinding setLayoutBinding {};
				setLayoutBinding.descriptorType = type;// type of descriptor ( uniform, dynamic uniform, image sampler, etc)
				setLayoutBinding.stageFlags = stageFlags; // Shader stage to bind to
				setLayoutBinding.binding = binding;// Binding point in shader (designated by binding number in shader)
				setLayoutBinding.descriptorCount = descriptorCount;// Number of descriptors for binding	

				setLayoutBinding.pImmutableSamplers = nullptr;							// For texture : can make sampler immutable by specifiying in layout
				return setLayoutBinding;
			}

			inline VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
				const VkDescriptorSetLayoutBinding* pBindings,
				uint32_t bindingCount)
			{
				VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {};
				descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				descriptorSetLayoutCreateInfo.pBindings = pBindings;// array of binding infos
				descriptorSetLayoutCreateInfo.bindingCount = bindingCount;// number of binding infos
				return descriptorSetLayoutCreateInfo;
			}

			inline VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo()
			{
				VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo {};
				pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				return pipelineVertexInputStateCreateInfo;
			}

			inline VkVertexInputBindingDescription vertexInputBindingDescription(
				uint32_t binding,
				uint32_t stride,
				VkVertexInputRate inputRate)
			{
				VkVertexInputBindingDescription vInputBindDescription {};
				vInputBindDescription.binding = binding;//can bind multiple streams of data, this defines which one
				vInputBindDescription.stride = stride; //size of a single vertex object in memory
				vInputBindDescription.inputRate = inputRate;//how to move between data after each vertex
															//VK_VERTEX_INPUT_RATE_INSTANCE : move to a vertex for the next instance
				return vInputBindDescription;
			}

			inline VkVertexInputAttributeDescription vertexInputAttributeDescription(
				uint32_t binding,
				uint32_t location,
				VkFormat format,
				uint32_t offset)
			{
				VkVertexInputAttributeDescription vInputAttribDescription {};
				vInputAttribDescription.location = location;//location in shader where data will be read from
				vInputAttribDescription.binding = binding; //which binding the data is at  ( should be same as above)
				vInputAttribDescription.format = format; // format the data will take (also helps define the size of the data) 
				vInputAttribDescription.offset = offset; // where this attribute is defined in the data for a single vertex
				return vInputAttribDescription;
			}


			inline VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo(
				const std::vector<VkVertexInputBindingDescription> &vertexBindingDescriptions,
				const std::vector<VkVertexInputAttributeDescription> &vertexAttributeDescriptions
			)
			{
				VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
				pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindingDescriptions.size());
				pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = vertexBindingDescriptions.data(); //list of vertext binding descriptions (data spacing / stride info)
				pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size());
				pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data(); // list of vertext attribute descriptions (data format and wheret to bind to/from)
				return pipelineVertexInputStateCreateInfo;
			}

			inline VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(
				VkPrimitiveTopology topology,
				VkPipelineInputAssemblyStateCreateFlags flags,
				VkBool32 primitiveRestartEnable)
			{
				VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo {};
				pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
				pipelineInputAssemblyStateCreateInfo.topology = topology;// primitive type to assemble vertices as
																		 //allow overriding of strip topology to start new primitives
				pipelineInputAssemblyStateCreateInfo.flags = flags;
				pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = primitiveRestartEnable;
				return pipelineInputAssemblyStateCreateInfo;
			}

			inline VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(
				VkPolygonMode polygonMode,
				VkCullModeFlags cullMode,
				VkFrontFace frontFace,
				VkPipelineRasterizationStateCreateFlags flags = 0)
			{
				VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo{};
				pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
				pipelineRasterizationStateCreateInfo.polygonMode = polygonMode;			// how to handle filling points between vertices								
				pipelineRasterizationStateCreateInfo.cullMode = cullMode;				// which face of the triangle to cull (backface cull)								
				pipelineRasterizationStateCreateInfo.frontFace = frontFace;				// Winding to determine which side is front								
				pipelineRasterizationStateCreateInfo.flags = flags;														
				pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;		// change if fragments beyond/near the far planes are clipped(default) or clamped.. must enable depth clamp in device features								
				pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;					// how thick lines should be when drawn	

				pipelineRasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;			// Wheter to add debt biast to fragments. (good for stoppging "shadow acne" in shadow mapping)
				pipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;	// wheter to desicard data and skip rasterizer. never creates fragments, only suitable for pipeline without framebuffer output.
				return pipelineRasterizationStateCreateInfo;							
			}			

			inline VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo(
				VkSampleCountFlagBits rasterizationSamples,
				VkPipelineMultisampleStateCreateFlags flags = 0)
			{
				VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo {};
				pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
				pipelineMultisampleStateCreateInfo.rasterizationSamples = rasterizationSamples; //number of samples to use per fragment;
				pipelineMultisampleStateCreateInfo.flags = flags;
				return pipelineMultisampleStateCreateInfo;
			}

			inline VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState(
				VkColorComponentFlags colorWriteMask,
				VkBool32 blendEnable)
			{
				VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState {};
				pipelineColorBlendAttachmentState.colorWriteMask = colorWriteMask; //colors to apply blending to
				pipelineColorBlendAttachmentState.blendEnable = blendEnable;
				return pipelineColorBlendAttachmentState;
			}

			inline VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(
				uint32_t attachmentCount,
				const VkPipelineColorBlendAttachmentState * pAttachments)
			{
				VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo {};
				pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
				pipelineColorBlendStateCreateInfo.attachmentCount = attachmentCount;
				pipelineColorBlendStateCreateInfo.pAttachments = pAttachments;
				pipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE; // alternative to calculations is to use logical operations
				return pipelineColorBlendStateCreateInfo;
			}

			inline VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(
				const std::vector<VkDynamicState>& pDynamicStates,
				VkPipelineDynamicStateCreateFlags flags = 0)
			{
				VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
				//dynamic viewport : can resize in command buffer with vkCmdSetViewport(commandbuffer,0,1,&viewport);
				//dynamic viewport : can resize in command buffer with vkCmdSetScissor(commandbuffer,0,1,&scissor);
				pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
				pipelineDynamicStateCreateInfo.pDynamicStates = pDynamicStates.data();
				pipelineDynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(pDynamicStates.size());
				pipelineDynamicStateCreateInfo.flags = flags;
				return pipelineDynamicStateCreateInfo;
			}

			inline VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo(
				VkBool32 depthTestEnable,
				VkBool32 depthWriteEnable,
				VkCompareOp depthCompareOp)
			{
				VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo {};
				pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
				pipelineDepthStencilStateCreateInfo.depthTestEnable = depthTestEnable; //enable checking depth to determine fragment write
				pipelineDepthStencilStateCreateInfo.depthWriteEnable = depthWriteEnable;// enable writing to depth buffer (to replace old values)
				pipelineDepthStencilStateCreateInfo.depthCompareOp = depthCompareOp;// comparison operation that allows an overwrite (is in front)
				pipelineDepthStencilStateCreateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
				pipelineDepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;		// Depth bounds test: does the depth value exist between 2 bounds
				pipelineDepthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;			// Enable stencil test
				return pipelineDepthStencilStateCreateInfo;
			}

			inline VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo(
				uint32_t viewportCount,
				uint32_t scissorCount,
				VkPipelineViewportStateCreateFlags flags = 0)
			{
				VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo {};
				pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
				pipelineViewportStateCreateInfo.viewportCount = viewportCount;
				pipelineViewportStateCreateInfo.scissorCount = scissorCount;
				pipelineViewportStateCreateInfo.flags = flags;
				return pipelineViewportStateCreateInfo;
			}

			inline VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(
				const VkDescriptorSetLayout* pSetLayouts,
				uint32_t setLayoutCount = 1)
			{
				VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
				pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
				pipelineLayoutCreateInfo.pSetLayouts = pSetLayouts;
				return pipelineLayoutCreateInfo;
			}

			inline VkGraphicsPipelineCreateInfo pipelineCreateInfo(
				VkPipelineLayout layout,
				VkRenderPass renderPass,
				VkPipelineCreateFlags flags = 0)
			{
				VkGraphicsPipelineCreateInfo pipelineCreateInfo {};
				pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
				pipelineCreateInfo.layout = layout;         //pipeline layout pipeline should use
				pipelineCreateInfo.renderPass = renderPass;	//render pass description the pipeline is compatible with
				pipelineCreateInfo.flags = flags;

				pipelineCreateInfo.subpass = 0;	//subpass of rneder pass to use with pipeline
				// pipeline derivatives : can create multiple pipels that derive from one another for optimization
				pipelineCreateInfo.basePipelineIndex = -1; //or index of pipeline being created to derive from (in case creating multiple at once)
				pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;  //existing pipeline to derive from
				return pipelineCreateInfo;
			}

		}
	}

}

