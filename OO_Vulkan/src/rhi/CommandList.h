/************************************************************************************//*!
\file           CommandList.h
\project        Ouroboros
\author         Jamie Kong, j.kong, 390004720 | code contribution (100%)
\par            email: j.kong\@digipen.edu
\date           Oct 02, 2022
\brief              Declares a command list RHI to keep track of state

Copyright (C) 2022 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*//*************************************************************************************/
#pragma once

#include "MathCommon.h"
#include <array>
#include <vulkan/vulkan.h>
#include "VulkanTexture.h"
#include "DescriptorBuilder.h"
#include "RGResource.h"

namespace rhi
{
	class CommandList;

	class DescriptorSetInfo {
	public:
		DescriptorBuilder builder;
		VkShaderStageFlags shaderStage;
		VkDescriptorSetLayout layout;
		VkDescriptorSet descriptor;

		bool built = false;
		bool expected = false;
		bool bound = false;

		bool hasDynamicOffset = false;
		uint32_t dynamicOffset;
		
		CommandList* m_cmdList;

		DescriptorSetInfo& BindImage(uint32_t binding, vkutils::Texture* texture, VkDescriptorType type);
		DescriptorSetInfo& BindImage(uint32_t binding, vkutils::Texture* texture, VkDescriptorType type, VkImageView viewOverride);
		DescriptorSetInfo& BindImage(uint32_t binding, VkDescriptorImageInfo* imageInfo, VkDescriptorType type, uint32_t count = 1);
		DescriptorSetInfo& BindSampler(uint32_t binding, VkSampler sampler, VkShaderStageFlags stageFlagsInclude = 0);
		DescriptorSetInfo& BindBuffer(uint32_t binding, const VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type,ResourceUsage access = ResourceUsage::SRV, VkShaderStageFlags stageFlagsInclude = 0);
		DescriptorSetInfo& SetDynamicOffset(uint32_t binding, uint32_t offset);
	};

// Another better alternative is to use Vulkan HPP.
class CommandList
{
public:
	

	CommandList(const VkCommandBuffer& cmd, const char* name = nullptr, const glm::vec4 col = glm::vec4{ 1.0f,1.0f,1.0f,0.0f });
	~CommandList();

	void BeginNameRegion(const char* name, const glm::vec4 col = glm::vec4{ 1.0f,1.0f,1.0f,0.0f });
	void EndNamedRegion();

	ImageStateTracking* BeginTrackingImage(vkutils::Texture* tex);
	ImageStateTracking* getTrackedImage(vkutils::Texture* tex);
	ImageStateTracking* ensureTrackedImage(vkutils::Texture* tex);

	BufferStateTracking* BeginTrackingBuffer(VkBuffer buffer);
	BufferStateTracking* getTrackedBuffer(VkBuffer buffer);
	BufferStateTracking* ensureTrackedBuffer(VkBuffer buffer);

	void VerifyResourceStates();
	void RestoreResourceStates();

	void VerifyImageResourceStates();
	void RestoreImageResourceStates();
	void VerifyBufferResourceStates();
	void RestoreBufferResourceStates();

	void CopyImage(vkutils::Texture* src, vkutils::Texture* dst);
	void BlitImage(vkutils::Texture* src, vkutils::Texture* dst);

	//----------------------------------------------------------------------------------------------------
	// Binding Commands
	//----------------------------------------------------------------------------------------------------
	void BindAttachment(uint32_t bindPoint, vkutils::Texture* tex, bool clearOnDraw = false);
	void BindDepthAttachment(vkutils::Texture* tex, bool clearOnDraw = false);

	void BindVertexBuffer(
		uint32_t firstBinding,
		uint32_t bindingCount,
		const VkBuffer* pBuffers,
		const VkDeviceSize* pOffsets = nullptr);

	void BindIndexBuffer(
		VkBuffer buffer,
		VkDeviceSize offset,
		VkIndexType indexType
	);

	void BeginRendering(VkRect2D renderArea);
	void EndRendering();

	void BindPSO(const VkPipeline& pso, VkPipelineLayout pipelay, const VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS);
	void BindPSO(std::string vertex, std::string fragment);
	void BindPSO(std::string compute);

	void SetPushConstant(VkPipelineLayout layout
		, VkDeviceSize size, const void* data
		, VkDeviceSize offset = 0);

	DescriptorSetInfo& DescriptorSetBegin(uint32_t set);

	void BindDescriptorSet(uint32_t set,uint32_t binding, VkDescriptorSet descriptor, VkDescriptorSetLayout setLayout);

	void BindDescriptorSet(
		VkPipelineLayout layout,
		uint32_t firstSet,
		uint32_t descriptorSetCount,
		const VkDescriptorSet* pDescriptorSets,
		VkPipelineBindPoint bindpoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		uint32_t dynamicOffsetCount = 0,
		const uint32_t* pDynamicOffsets = nullptr
	);

	template<typename T_ARRAY>
	void BindDescriptorSet(
		VkPipelineLayout layout,
		uint32_t firstSet,
		const T_ARRAY& array,
		VkPipelineBindPoint bindpoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		uint32_t dynamicOffsetCount = 1,
		const uint32_t* pDynamicOffsets = nullptr
	)
	{
		this->BindDescriptorSet(
			layout,
			firstSet,
			(uint32_t)array.size(),
			array.data(),
			bindpoint,
			dynamicOffsetCount,
			pDynamicOffsets);
	}

	//----------------------------------------------------------------------------------------------------
	// Drawing Commands
	//----------------------------------------------------------------------------------------------------
	void ClearImage(vkutils::Texture* tex, VkClearValue clear);

	void Draw(
		uint32_t vertexCount,
		uint32_t instanceCount,
		uint32_t firstVertex = 0,
		uint32_t firstInstance = 0);

	void DrawIndexed(
		uint32_t indexCount,
		uint32_t instanceCount,
		uint32_t firstIndex = 0,
		int32_t vertexOffset = 0,
		uint32_t firstInstance = 0);
	

	void DrawIndexedIndirect(
			VkBuffer buffer,
			VkDeviceSize offset,
			uint32_t drawCount,
			uint32_t stride = sizeof(VkDrawIndexedIndirectCommand));
	
	// Helper function to draw a Full Screen Quad, without binding vertex and index buffers.
	void DrawFullScreenQuad();

	void Dispatch(uint32_t x, uint32_t y = 1 , uint32_t z = 1);

	//----------------------------------------------------------------------------------------------------
	// Pipeline State Commands
	//----------------------------------------------------------------------------------------------------

	void SetDefaultViewportAndScissor();

	void SetViewport(uint32_t firstViewport,
		uint32_t viewportCount,
		const VkViewport* pViewports);

	void SetViewport(const VkViewport& viewport);

	void SetScissor(uint32_t firstScissor,
		uint32_t scissorCount,
		const VkRect2D* pScissors);

	void SetScissor(const VkRect2D& scissor);

	// TODO: Function not here? Add it on demand...

	VkCommandBuffer getCommandBuffer();
private:
	void PrepareDescriptors();
	void CommitDescriptors();
	void DenoteStateChanged();
	void EndIfRendering();

	void GetOrBuildPipeline();

	enum
	{
		VERTEX,
		FRAGMENT,
		COMPUTE
	};

	VkCommandBuffer m_VkCommandBuffer{};

	VkPipelineLayout m_pipeLayout{};
	VkPipeline m_pipeline{};
	VkPipelineBindPoint m_pipelineBindPoint{ VK_PIPELINE_BIND_POINT_MAX_ENUM };
	VkShaderStageFlags m_targetStage{ VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM };

	//pipeline info
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = oGFX::vkutils::inits::pipelineInputAssemblyStateCreateInfo();
	VkPipelineRasterizationStateCreateInfo rasterizationState = oGFX::vkutils::inits::pipelineRasterizationStateCreateInfo();
	VkPipelineColorBlendAttachmentState blendAttachmentState = oGFX::vkutils::inits::pipelineColorBlendAttachmentState();
	VkPipelineColorBlendStateCreateInfo colorBlendState = oGFX::vkutils::inits::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
	VkPipelineDepthStencilStateCreateInfo depthStencilState = oGFX::vkutils::inits::pipelineDepthStencilStateCreateInfo();
	VkPipelineViewportStateCreateInfo viewportState = oGFX::vkutils::inits::pipelineViewportStateCreateInfo();
	VkPipelineMultisampleStateCreateInfo multisampleState = oGFX::vkutils::inits::pipelineMultisampleStateCreateInfo();
	std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState = oGFX::vkutils::inits::pipelineDynamicStateCreateInfo(dynamicStateEnables);

	VkGraphicsPipelineCreateInfo pipelineCI;

	std::vector<VkVertexInputBindingDescription> bindingDescription = oGFX::GetGFXVertexInputBindings();
	std::vector<VkVertexInputAttributeDescription>attributeDescriptions = oGFX::GetGFXVertexInputAttributes();

	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = oGFX::vkutils::inits::pipelineVertexInputStateCreateInfo(bindingDescription, attributeDescriptions);

	std::array< std::string, 3 > shadercodes;

	VkComputePipelineCreateInfo computeCI;
	// end pipeline info

	std::array<VkRect2D, 8> m_scissor;
	std::array<VkViewport, 8> m_viewport;
	std::array<VkRenderingAttachmentInfo, 8> m_attachments{};
	std::array<VkFormat, 8> m_attachmentFormats{};
	std::array<bool, 8> m_shouldClearAttachment{};
	int32_t m_highestAttachmentBound{-1};
	bool m_depthBound = false;
	bool m_shouldClearDepth = false;
	VkRenderingAttachmentInfo m_depth{};
	VkFormat m_depthFormat{VK_FORMAT_UNDEFINED};
	float m_push_constant[128 / sizeof(float)]{0.0f};
	bool m_regionNamed = false;

	VkRect2D m_renderArea{};
	bool m_attachmentReady{ false };
	bool m_currentlyRendering{ false };

	std::unordered_map<vkutils::Texture*, ImageStateTracking> m_trackedTextures;
	std::unordered_map<VkBuffer, BufferStateTracking> m_trackedBuffers;

	std::array<DescriptorSetInfo, 4> descriptorSets; // only support 4 sets


	uint32_t setCount{};
	uint32_t firstSet{ UINT32_MAX };
	std::array<VkDescriptorSet, 4> sets{};

	uint32_t dynamicOffsetCnt{};
	std::array<uint32_t, 4> dynOffsets{};
	 
	// TODO: Handle VK_PIPELINE_BIND_POINT_GRAPHICS etc nicely next time.
	// TODO: Maybe we can cache the stuff that is bound, for easier debugging, else taking GPU captures is really unproductive.
};

}
