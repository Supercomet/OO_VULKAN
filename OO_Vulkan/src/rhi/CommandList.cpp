/************************************************************************************//*!
\file           CommandList.cpp
\project        Ouroboros
\author         Jamie Kong, j.kong, 390004720 | code contribution (100%)
\par            email: j.kong\@digipen.edu
\date           Oct 02, 2022
\brief               Defines a command list RHI to keep track of state

Copyright (C) 2022 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*//*************************************************************************************/
#include "CommandList.h"

#include "VulkanRenderer.h"
#include <cassert>

namespace rhi
{

	inline VkShaderStageFlags getStage(VkPipelineBindPoint bp) {
		return bp == VK_PIPELINE_BIND_POINT_COMPUTE ? VK_SHADER_STAGE_COMPUTE_BIT : VK_SHADER_STAGE_ALL_GRAPHICS;
	}

	void CommandList::BindPSO(const VkPipeline& pso, VkPipelineLayout pipelay, const VkPipelineBindPoint bindPoint)
	{
		m_pipelineBindPoint = bindPoint;
		m_pipeLayout = pipelay;
		m_targetStage = getStage(m_pipelineBindPoint);

		vkCmdBindPipeline(m_VkCommandBuffer, bindPoint, pso);
	}

	void CommandList::SetPushConstant(VkPipelineLayout layout, const VkPushConstantRange& pcr, const void* data)
	{
		memset(m_push_constant, 0, 128);
		memcpy(m_push_constant, data, pcr.size);
		vkCmdPushConstants(m_VkCommandBuffer, layout, VK_SHADER_STAGE_ALL, pcr.offset, pcr.size, data);
	}

	DescriptorSetInfo& CommandList::DescriptorSetBegin(uint32_t set)
	{
		assert(set < 4);
		assert(m_pipelineBindPoint != VK_PIPELINE_BIND_POINT_MAX_ENUM);
		descriptorSets[set].builder = DescriptorBuilder::Begin();

		
		descriptorSets[set].shaderStage = m_targetStage;

		descriptorSets[set].expected = true;
		descriptorSets[set].bound = false;
		descriptorSets[set].built = false;

		return descriptorSets[set];
	}

	void CommandList::BindDescriptorSet(uint32_t set, VkDescriptorSet descriptor)
	{

	}

	CommandList::CommandList(const VkCommandBuffer& cmd, const char* name, const glm::vec4 col)
		: m_VkCommandBuffer{ cmd }
	{
		BeginNameRegion(name, col);

		for (size_t i = 0; i < descriptorSets.size(); i++)
		{
			descriptorSets[i].m_cmdList = this;
		}

		for (auto& a : m_attachments)
		{
			a = VkRenderingAttachmentInfo{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
		}
	}
	CommandList::~CommandList()
	{
		RestoreImageResourceStates();
		EndNamedRegion();
	}

	void CommandList::EndNamedRegion()
	{
		auto region = VulkanRenderer::get()->pfnDebugMarkerRegionEnd;

		if (region && m_regionNamed)
		{
			region(m_VkCommandBuffer);
		}
		m_regionNamed = false;
	}

	void CommandList::BeginTrackingImage(vkutils::Texture* tex)
	{
		auto iter = m_trackedTextures.find(tex);
		if (iter == m_trackedTextures.end())
		{
			//printf("  Tracking %s ..\n", tex->name.c_str());
			ResourceStateTracking state;
			OO_ASSERT(tex->referenceLayout == tex->currentLayout);
			state.referenceLayout = tex->referenceLayout;
			state.currentLayout = tex->referenceLayout;
			m_trackedTextures[tex] = state;

		}
	}

	ResourceStateTracking* CommandList::getTrackedImage(vkutils::Texture* tex)
	{
		auto iter = m_trackedTextures.find(tex);
		if (iter != m_trackedTextures.end()) {
			return &iter->second;
		}
		return nullptr;
	}

	ResourceStateTracking* CommandList::ensureTrackedImage(vkutils::Texture* tex)
	{
		ResourceStateTracking* result = getTrackedImage(tex);
		if (result == nullptr)
		{
			BeginTrackingImage(tex);
			result = getTrackedImage(tex);
		}
		return result;
	}

	void CommandList::VerifyImageResourceStates()
	{
		for (auto& [tex, state] : m_trackedTextures) 
		{
			if (state.expectedLayout != state.currentLayout) 
			{
				// TODO: Batch together barriers
				vkutils::TransitionImage(m_VkCommandBuffer, *tex, state.currentLayout, state.expectedLayout);
				state.currentLayout = state.expectedLayout;
			}
			else 
			{
				//printf("\tTransition::%s is already in %s\n", tex->name.c_str(), oGFX::vkutils::tools::VkImageLayoutString(state.expectedLayout).c_str());
			}
		}
	}

	void CommandList::RestoreImageResourceStates()
	{
		for (auto& [tex, state] : m_trackedTextures)
		{
			if (state.referenceLayout != state.currentLayout)
			{
				// TODO: Batch together barriers
				vkutils::TransitionImage(m_VkCommandBuffer, *tex, state.currentLayout, state.referenceLayout);
				state.currentLayout = state.referenceLayout;
			}
		}
	}

	void CommandList::CopyImage(vkutils::Texture* src, vkutils::Texture* dst)
	{
		OO_ASSERT(src != nullptr && dst != nullptr);

		const ResourceStateTracking* dstTrack = ensureTrackedImage(dst);
		const ResourceStateTracking* srcTrack = ensureTrackedImage(src);
		// we will use and restore the tracked states

		VkImageSubresourceLayers srcCopy{ VK_IMAGE_ASPECT_COLOR_BIT ,0,0,1 };
		srcCopy.layerCount = src->layerCount;
		if (src->format == VulkanRenderer::G_DEPTH_FORMAT) {
			srcCopy.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		VkImageSubresourceLayers dstCopy{ VK_IMAGE_ASPECT_COLOR_BIT ,0,0,1 };
		dstCopy.layerCount = src->layerCount;
		if (dst->format == VulkanRenderer::G_DEPTH_FORMAT) {
			dstCopy.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		vkutils::ComputeImageBarrier(m_VkCommandBuffer, *src,srcTrack->currentLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		vkutils::ComputeImageBarrier(m_VkCommandBuffer, *dst,dstTrack->currentLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		VkImageCopy region{};
		region.srcSubresource = srcCopy;
		region.srcOffset = {};
		region.dstSubresource = dstCopy;
		region.dstOffset = {};
		region.extent = { src->width,src->height,1 };
		vkCmdCopyImage(m_VkCommandBuffer,src->image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			, dst->image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &region);
		vkutils::ComputeImageBarrier(m_VkCommandBuffer, *src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcTrack->currentLayout);
		vkutils::ComputeImageBarrier(m_VkCommandBuffer, *dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstTrack->currentLayout);

		//dont need to modify tracked state because we have returned it
	}

void CommandList::BeginNameRegion(const char* name, const glm::vec4 col)
{
	auto region = VulkanRenderer::get()->pfnDebugMarkerRegionBegin;
	if (name && region)
	{
		EndNamedRegion();

		VkDebugMarkerMarkerInfoEXT marker = {};
		marker.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
		memcpy(marker.color, &col[0], sizeof(float) * 4);
		marker.pMarkerName = name;
		region(m_VkCommandBuffer, &marker);
		m_regionNamed = true;

		VK_NAME(VulkanRenderer::get()->m_device.logicalDevice, name, m_VkCommandBuffer);
	}
}

void CommandList::BindAttachment(uint32_t bindPoint, vkutils::Texture* tex, bool clearOnDraw)
{

	if (tex) {
		//start tracking
		ResourceStateTracking* tracked = ensureTrackedImage(tex);		
		tracked->expectedLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


		VkRenderingAttachmentInfo albedoInfo{};
		albedoInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		albedoInfo.pNext = NULL;
		albedoInfo.resolveMode = {};
		albedoInfo.resolveImageView = {};
		albedoInfo.resolveImageLayout = {};
		albedoInfo.imageView = tex->view;
		albedoInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		albedoInfo.loadOp = clearOnDraw ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		albedoInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		albedoInfo.clearValue = VkClearValue{ {} };

		m_attachments[bindPoint] = albedoInfo;
		m_highestAttachmentBound = std::max<int32_t>(bindPoint, m_highestAttachmentBound);

		m_renderArea.extent = { tex->width, tex->height };
	}
	else 
	{
		//bind null
		m_attachments[bindPoint] = VkRenderingAttachmentInfo{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR };
	}

}

void CommandList::BindDepthAttachment(vkutils::Texture* tex, bool clearOnDraw)
{
	if (tex) 
	{
		//start tracking
		ResourceStateTracking* tracked = ensureTrackedImage(tex);
		tracked->expectedLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkRenderingAttachmentInfo depthInfo{};
		depthInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		depthInfo.pNext = NULL;
		depthInfo.resolveMode = {};
		depthInfo.resolveImageView = {};
		depthInfo.resolveImageLayout = {};
		depthInfo.imageView = tex->view;
		depthInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthInfo.loadOp = clearOnDraw ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		depthInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthInfo.clearValue = VkClearValue{ {} };
		m_depth = depthInfo;
		m_depthBound = true;
	}
	else 
	{
		m_depthBound = false;
	}
}
void CommandList::BindVertexBuffer(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets /*= nullptr*/)
{
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(m_VkCommandBuffer, firstBinding, bindingCount, pBuffers, pOffsets ? pOffsets : offsets);
}

void CommandList::BindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
{
	vkCmdBindIndexBuffer(m_VkCommandBuffer, buffer, offset, indexType);
}

void CommandList::BeginRendering(VkRect2D renderArea)
{

	VerifyImageResourceStates();

	VkRenderingInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.renderArea = renderArea;
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = m_highestAttachmentBound + 1; // should be [0-8];
	renderingInfo.pColorAttachments = m_attachments.data();

	renderingInfo.pDepthAttachment = m_depthBound ? &m_depth : NULL;
	renderingInfo.pStencilAttachment = m_depthBound ? &m_depth : NULL;

	vkCmdBeginRendering(m_VkCommandBuffer, &renderingInfo);
}

void CommandList::EndRendering()
{
	vkCmdEndRendering(m_VkCommandBuffer);
}


void CommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
	CommitDescriptors();
	
	BeginRendering(m_renderArea);

	vkCmdDraw(m_VkCommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
	EndRendering();
}

void CommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
	CommitDescriptors();
	
	BeginRendering(m_renderArea);
	vkCmdDrawIndexed(m_VkCommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	EndRendering();
}

void CommandList::DrawIndexedIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
	CommitDescriptors();

	BeginRendering(m_renderArea);
	::DrawIndexedIndirect(m_VkCommandBuffer, buffer, offset, drawCount, stride);
	EndRendering();
}

void CommandList::BindDescriptorSet(VkPipelineLayout layout,
	uint32_t firstSet, uint32_t descriptorSetCount,
	const VkDescriptorSet* pDescriptorSets,
	VkPipelineBindPoint bindpoint,
	uint32_t dynamicOffsetCount /*= 1*/, const uint32_t* pDynamicOffsets /*= nullptr */)
{
	m_pipeLayout = layout;

	for (size_t i = 0; i < descriptorSetCount; i++)
	{
		DescriptorSetInfo& descSet  = descriptorSets[i + firstSet];
		descSet.expected = true;
		descSet.built = true;
		descSet.bound = false;
		descSet.descriptor = pDescriptorSets[i];
	}

	for (size_t i = 0; i < dynamicOffsetCount; i++)
	{
		DescriptorSetInfo& descSet = descriptorSets[i + firstSet];
		if (pDynamicOffsets) {
			descSet.hasDynamicOffset = true;
			descSet.dynamicOffset = pDynamicOffsets[i];
		}
	}

	uint32_t dynamicOffset = 0;
	// vkCmdBindDescriptorSets(
	// 	m_VkCommandBuffer,
	// 	bindpoint,
	// 	layout,
	// 	firstSet,
	// 	descriptorSetCount,
	// 	pDescriptorSets,
	// 	dynamicOffsetCount,
	// 	pDynamicOffsets ? pDynamicOffsets : &dynamicOffset);
}

void CommandList::DrawFullScreenQuad()
{
	CommitDescriptors();

	BeginRendering(m_renderArea);
	vkCmdDraw(m_VkCommandBuffer, 3, 1, 0, 0);
	EndRendering();
}

void CommandList::Dispatch(uint32_t x, uint32_t y, uint32_t z)
{
	VerifyImageResourceStates();
	CommitDescriptors();
	vkCmdDispatch(m_VkCommandBuffer, x, y, z);
}

void CommandList::SetDefaultViewportAndScissor()
{
	::SetDefaultViewportAndScissor(m_VkCommandBuffer,&m_viewport.front(),&m_scissor.front());
}

void CommandList::SetViewport(uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports)
{
	assert(viewportCount < 8);
	for (size_t i = 0; i < viewportCount; i++)
	{
		m_viewport[i] = pViewports[i];
	}
	vkCmdSetViewport(m_VkCommandBuffer, firstViewport, viewportCount, pViewports);
}

void CommandList::SetViewport(const VkViewport& viewport)
{
	const VkViewport vp{ viewport };
	this->SetViewport(0, 1, &vp);
}

void CommandList::SetScissor(uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors)
{
	assert(scissorCount < 8);
	for (size_t i = 0; i < scissorCount; i++)
	{
		m_scissor[i] = pScissors[i];
	}
	vkCmdSetScissor(m_VkCommandBuffer, firstScissor, scissorCount, pScissors);
}

void CommandList::SetScissor(const VkRect2D& scissor)
{
	const VkRect2D s{ scissor };
	this->SetScissor(0, 1, &s);
}

void CommandList::CommitDescriptors()
{
	uint32_t count{};
	uint32_t firstSet{ UINT32_MAX };
	std::array<VkDescriptorSet, 4> sets{};

	uint32_t dynamicOffsetCnt{};
	std::array<uint32_t, 4> dynOffsets{};

	for (size_t i = 0; i < descriptorSets.size(); i++)
	{
		DescriptorSetInfo& descSet = descriptorSets[i];
		if (descSet.expected && descSet.bound == false) 
		{
			firstSet = std::min(count, firstSet);

			if (descSet.built == false)
			{
				descSet.builder.Build(descSet.descriptor, descSet.layout);
				descSet.built = true;
			}

			if (descSet.hasDynamicOffset)
			{
				dynOffsets[dynamicOffsetCnt] = descSet.dynamicOffset;
				dynamicOffsetCnt++;
			}

			sets[count] = descSet.descriptor;
			descSet.bound = true;
			count++;
		}
	}

	if (count == 0) 
		return;

	uint32_t dynamicOffset = 0;
	vkCmdBindDescriptorSets(
		m_VkCommandBuffer,
		m_pipelineBindPoint,
		m_pipeLayout,
		firstSet,
		count,
		sets.data(),
		dynamicOffsetCnt,
		dynamicOffsetCnt ? dynOffsets.data() : nullptr);
}

DescriptorSetInfo& DescriptorSetInfo::BindImage(uint32_t binding, vkutils::Texture* texture, VkDescriptorType type, VkShaderStageFlags stageFlagsInclude)
{	
	VkImageLayout layout = (type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkDescriptorImageInfo texinfo = oGFX::vkutils::inits::descriptorImageInfo(
		VK_NULL_HANDLE, // no sampler
		texture->view,
		layout);

	builder.BindImage(binding, &texinfo, type, shaderStage | stageFlagsInclude);

	this->m_cmdList->BeginTrackingImage(texture);
	ResourceStateTracking* tracked = m_cmdList->getTrackedImage(texture);
	tracked->expectedLayout = layout;

	return *this;

}

DescriptorSetInfo& DescriptorSetInfo::BindSampler(uint32_t binding, VkSampler sampler, VkShaderStageFlags stageFlagsInclude)
{
	VkDescriptorImageInfo samplerInfo = oGFX::vkutils::inits::descriptorImageInfo(
		sampler, 
		VK_NULL_HANDLE,
		VK_IMAGE_LAYOUT_UNDEFINED);

	builder.BindImage(binding, &samplerInfo, VK_DESCRIPTOR_TYPE_SAMPLER , shaderStage | stageFlagsInclude);
	return *this;
}

DescriptorSetInfo& DescriptorSetInfo::BindBuffer(uint32_t binding, const VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlagsInclude)
{
	builder.BindBuffer(binding, bufferInfo, type, shaderStage | stageFlagsInclude);
	return *this;
}

}
