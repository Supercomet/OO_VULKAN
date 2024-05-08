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
		PROFILE_SCOPED();
		m_pipeline = pso;
		m_pipelineBindPoint = bindPoint;
		m_pipeLayout = pipelay;
		m_targetStage = getStage(m_pipelineBindPoint);

		vkCmdBindPipeline(m_VkCommandBuffer, bindPoint, pso);
	}

	void CommandList::BindPSO(std::string vertex, std::string fragment)
	{
		m_pipeline = VK_NULL_HANDLE;
		m_pipeLayout = VK_NULL_HANDLE;

		m_pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		shadercodes[VERTEX] = std::move(vertex);
		shadercodes[FRAGMENT] = std::move(fragment);
	}

	void CommandList::BindPSO(std::string compute)
	{
		m_pipeline = VK_NULL_HANDLE;
		m_pipeLayout = VK_NULL_HANDLE;

		m_pipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
		shadercodes[COMPUTE] = std::move(compute);
	}

	void CommandList::SetPushConstant(VkPipelineLayout layout, VkDeviceSize size, const void* data, VkDeviceSize offset)
	{
		PROFILE_SCOPED();
		memset(m_push_constant, 0, 128);
		memcpy(m_push_constant, data, size);
		vkCmdPushConstants(m_VkCommandBuffer, layout, VK_SHADER_STAGE_ALL, (uint32_t)offset, (uint32_t)size, data);
	}

	DescriptorSetInfo& CommandList::DescriptorSetBegin(uint32_t set)
	{
		assert(set < 4);
		assert(m_pipelineBindPoint != VK_PIPELINE_BIND_POINT_MAX_ENUM);
		DenoteStateChanged();

		descriptorSets[set].builder = DescriptorBuilder::Begin();
		
		descriptorSets[set].shaderStage = m_targetStage;

		descriptorSets[set].expected = true;
		descriptorSets[set].bound = false;
		descriptorSets[set].built = false;
		return descriptorSets[set];
	}

	void CommandList::BindDescriptorSet(uint32_t set,uint32_t binding, VkDescriptorSet descriptor, VkDescriptorSetLayout setLayout)
	{
		assert(set < 4);
		assert(descriptor != VK_NULL_HANDLE);
		assert(m_pipelineBindPoint != VK_PIPELINE_BIND_POINT_MAX_ENUM);
		DenoteStateChanged();

		descriptorSets[set].descriptor = descriptor;
		descriptorSets[set].layout = setLayout;

		descriptorSets[set].shaderStage = m_targetStage;
	
		descriptorSets[set].expected = true;
		descriptorSets[set].bound = false;
		descriptorSets[set].built = true;

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
		for (auto& f : m_attachmentFormats)
		{
			f = VK_FORMAT_UNDEFINED;
		}

		pipelineCI.pInputAssemblyState = &inputAssemblyState;
		pipelineCI.pRasterizationState = &rasterizationState;
		pipelineCI.pColorBlendState = &colorBlendState;
		pipelineCI.pMultisampleState = &multisampleState;
		pipelineCI.pViewportState = &viewportState;
		pipelineCI.pDepthStencilState = &depthStencilState;
		pipelineCI.pDynamicState = &dynamicState;
		//pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		//pipelineCI.pStages = shaderStages.data();
	}
	CommandList::~CommandList()
	{
		EndIfRendering();
		RestoreResourceStates();
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

	ImageStateTracking* CommandList::BeginTrackingImage(vkutils::Texture* tex)
	{
		OO_ASSERT(tex);

		auto iter = m_trackedTextures.find(tex);
		if (iter == m_trackedTextures.end())
		{
			//printf("  Tracking %s ..\n", tex->name.c_str());
			ImageStateTracking state;
			// this assert no longer works because we are now threaded
			//OO_ASSERT(tex->referenceLayout == tex->currentLayout);
			state.referenceLayout = tex->referenceLayout;
			state.currentLayout = tex->referenceLayout;
			m_trackedTextures[tex] = state;
		}
		return &m_trackedTextures[tex];
	}

	ImageStateTracking* CommandList::getTrackedImage(vkutils::Texture* tex)
	{
		auto iter = m_trackedTextures.find(tex);
		if (iter != m_trackedTextures.end()) {
			return &iter->second;
		}
		return nullptr;
	}

	ImageStateTracking* CommandList::ensureTrackedImage(vkutils::Texture* tex)
	{
		ImageStateTracking* result = getTrackedImage(tex);
		if (result == nullptr)
		{			
			result = BeginTrackingImage(tex);
		}
		return result;
	}

	BufferStateTracking* CommandList::BeginTrackingBuffer(VkBuffer buffer)
	{
		OO_ASSERT(buffer);

		auto iter = m_trackedBuffers.find(buffer);
		if (iter == m_trackedBuffers.end())
		{
			BufferStateTracking state;
			m_trackedBuffers[buffer] = state;
		}
		return &m_trackedBuffers[buffer];
	}

	BufferStateTracking* CommandList::getTrackedBuffer(VkBuffer buffer)
	{
		auto iter = m_trackedBuffers.find(buffer);
		if (iter != m_trackedBuffers.end()) {
			return &iter->second;
		}
		return nullptr;
	}

	BufferStateTracking* CommandList::ensureTrackedBuffer(VkBuffer buffer)
	{
		BufferStateTracking* result = getTrackedBuffer(buffer);
		if (result == nullptr)
		{			
			result = BeginTrackingBuffer(buffer);
		}
		return result;
	}

	void CommandList::VerifyResourceStates()
	{
		VerifyImageResourceStates();
		VerifyBufferResourceStates();

		PrepareDescriptors();
		GetOrBuildPipeline();
		CommitDescriptors();
		m_attachmentReady = true;
	}

	void CommandList::RestoreResourceStates()
	{
		RestoreImageResourceStates();
		RestoreBufferResourceStates();
	}

	void CommandList::VerifyImageResourceStates()
	{
		PROFILE_SCOPED();
		for (auto& [tex, state] : m_trackedTextures) 
		{
			if (state.expectedLayout != state.currentLayout) 
			{
				// TODO: Batch together barriers
				if (state.expectedLayout == VK_IMAGE_USAGE_STORAGE_BIT) {
					vkutils::ComputeImageBarrier(m_VkCommandBuffer, *tex, state.currentLayout, state.expectedLayout);
				}
				else {
					vkutils::TransitionImage(m_VkCommandBuffer, *tex, state.currentLayout, state.expectedLayout);
				}
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
		PROFILE_SCOPED();
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

	void CommandList::VerifyBufferResourceStates()
	{
		PROFILE_SCOPED();
		for (auto& [buffer, state] : m_trackedBuffers)
		{
			if (state.currentAccess != state.expectedAccess || state.expectedAccess == UAV)
			{
				VkAccessFlags srcAccess = state.currentAccess == UAV ? VK_ACCESS_MEMORY_WRITE_BIT : VK_ACCESS_MEMORY_READ_BIT;
				VkAccessFlags dstAccess = state.expectedAccess == UAV ? VK_ACCESS_MEMORY_WRITE_BIT : VK_ACCESS_MEMORY_READ_BIT;

				VkPipelineStageFlags prevStage = state.previousStage == VK_PIPELINE_BIND_POINT_COMPUTE ? VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT : VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
				VkPipelineStageFlags nextStage = m_pipelineBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE ? VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT : VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;

				// TODO: Batch together barriers
				oGFX::vkutils::tools::insertBufferMemoryBarrier(m_VkCommandBuffer, VulkanRenderer::get()->m_device.queueIndices.graphicsFamily,
					buffer, srcAccess, dstAccess,
					prevStage, nextStage);
				state.currentAccess = state.expectedAccess;
				state.previousStage = m_pipelineBindPoint;
			}
		}
	}

	void CommandList::RestoreBufferResourceStates()
	{
		for (auto& [buffer, state] : m_trackedBuffers)
		{
			if (state.currentAccess != state.expectedAccess || state.currentAccess == UAV)
			{
				VkAccessFlags srcAccess = state.currentAccess == UAV ? VK_ACCESS_MEMORY_WRITE_BIT : VK_ACCESS_MEMORY_READ_BIT;
				VkAccessFlags dstAccess = state.expectedAccess == UAV ? VK_ACCESS_MEMORY_WRITE_BIT : VK_ACCESS_MEMORY_READ_BIT;
				
				VkPipelineStageFlags prevStage = state.previousStage == VK_PIPELINE_BIND_POINT_COMPUTE ? VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT : VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
				VkPipelineStageFlags nextStage = m_pipelineBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE ? VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT : VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
				
				// TODO: Batch together barriers
				oGFX::vkutils::tools::insertBufferMemoryBarrier(m_VkCommandBuffer, VulkanRenderer::get()->m_device.queueIndices.graphicsFamily,
					buffer, srcAccess, dstAccess,
					prevStage, nextStage);
				state.currentAccess = state.referenceAccess;
			}
		}
	}

	void CommandList::CopyImage(vkutils::Texture* src, vkutils::Texture* dst)
	{
		OO_ASSERT(src != nullptr && dst != nullptr);

		if (src->format != dst->format) {
			BlitImage(src, dst);
			return;
		}

		const ImageStateTracking* dstTrack = ensureTrackedImage(dst);
		const ImageStateTracking* srcTrack = ensureTrackedImage(src);
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

	void CommandList::BlitImage(vkutils::Texture* src, vkutils::Texture* dst)
	{
		const ImageStateTracking* dstTrack = ensureTrackedImage(dst);
		const ImageStateTracking* srcTrack = ensureTrackedImage(src);

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

		vkutils::ComputeImageBarrier(m_VkCommandBuffer, *src, srcTrack->currentLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		vkutils::ComputeImageBarrier(m_VkCommandBuffer, *dst, dstTrack->currentLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		VkImageBlit region{};
		region.srcSubresource = srcCopy;
		region.srcOffsets[1] = VkOffset3D{ (int32_t)src->width, (int32_t)src->height,1 };
		region.dstSubresource = dstCopy;
		region.dstOffsets[1] = VkOffset3D{ (int32_t)dst->width, (int32_t)dst->height,1 };
		vkCmdBlitImage(m_VkCommandBuffer, src->image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			dst->image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &region, VkFilter::VK_FILTER_NEAREST);
		vkutils::ComputeImageBarrier(m_VkCommandBuffer, *src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcTrack->currentLayout);
		vkutils::ComputeImageBarrier(m_VkCommandBuffer, *dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstTrack->currentLayout);

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
		ImageStateTracking* tracked = ensureTrackedImage(tex);		
		tracked->expectedLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkRenderingAttachmentInfo albedoInfo{};
		albedoInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		albedoInfo.pNext = NULL;
		albedoInfo.resolveMode = {};
		albedoInfo.resolveImageView = {};
		albedoInfo.resolveImageLayout = {};
		albedoInfo.imageView = tex->view;
		albedoInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		//albedoInfo.loadOp = clearOnDraw ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		// handled by should clear draw
		albedoInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		albedoInfo.clearValue = VkClearValue{ {} };

		m_attachments[bindPoint] = albedoInfo;
		m_attachmentFormats[bindPoint] = tex->format;
		m_shouldClearAttachment[bindPoint] = clearOnDraw;

		m_highestAttachmentBound = std::max<int32_t>(bindPoint, m_highestAttachmentBound);

		m_renderArea.extent = { tex->width, tex->height };
	}
	else 
	{
		//bind null
		m_attachments[bindPoint] = VkRenderingAttachmentInfo{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR };
		m_attachmentFormats[bindPoint] = VK_FORMAT_UNDEFINED;
	}
	DenoteStateChanged();
}

void CommandList::BindDepthAttachment(vkutils::Texture* tex, bool clearOnDraw)
{
	if (tex) 
	{
		//start tracking
		ImageStateTracking* tracked = ensureTrackedImage(tex);
		tracked->expectedLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkRenderingAttachmentInfo depthInfo{};
		depthInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
		depthInfo.pNext = NULL;
		depthInfo.resolveMode = {};
		depthInfo.resolveImageView = {};
		depthInfo.resolveImageLayout = {};
		depthInfo.imageView = tex->view;
		depthInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		// depthInfo.loadOp = clearOnDraw ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		// handled by should clear
		depthInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthInfo.clearValue = VkClearValue{ {} };
		m_depth = depthInfo;
		m_depthBound = true;
		m_depthFormat = tex->format;
		m_shouldClearDepth = clearOnDraw;

		m_renderArea.extent.width = std::max(tex->width, m_renderArea.extent.width);
		m_renderArea.extent.height = std::max(tex->height, m_renderArea.extent.height);
	}
	else 
	{
		m_depthBound = false;
		m_depth = VkRenderingAttachmentInfo{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR };
		m_depthFormat = VK_FORMAT_UNDEFINED;
	}
	DenoteStateChanged();
}
void CommandList::BindVertexBuffer(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets /*= nullptr*/)
{
	PROFILE_SCOPED();
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(m_VkCommandBuffer, firstBinding, bindingCount, pBuffers, pOffsets ? pOffsets : offsets);
}

void CommandList::BindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
{
	PROFILE_SCOPED();
	vkCmdBindIndexBuffer(m_VkCommandBuffer, buffer, offset, indexType);
}

void CommandList::BeginRendering(VkRect2D renderArea)
{
	PROFILE_SCOPED();

	if (m_attachmentReady == true) {
		return;
	}
	else {
		EndIfRendering();
	}

	VerifyResourceStates();
	
	m_depth.loadOp = m_shouldClearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
	m_shouldClearDepth = false;
	for (size_t i = 0; i < size_t(m_highestAttachmentBound + 1); i++)
	{
		m_attachments[i].loadOp = m_shouldClearAttachment[i] ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		m_shouldClearAttachment[i] = false;
	}

	VkRenderingInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.renderArea = renderArea;
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = m_highestAttachmentBound + 1; // should be [0-8];
	renderingInfo.pColorAttachments = m_attachments.data();

	renderingInfo.pDepthAttachment = m_depthBound ? &m_depth : NULL;
	renderingInfo.pStencilAttachment = m_depthBound ? &m_depth : NULL;

	vkCmdBeginRendering(m_VkCommandBuffer, &renderingInfo);
	m_currentlyRendering = true;
}

void CommandList::EndRendering()
{
	PROFILE_SCOPED();
	vkCmdEndRendering(m_VkCommandBuffer);
	m_currentlyRendering = false;
}


void CommandList::ClearImage(vkutils::Texture* tex, VkClearValue clear)
{
	OO_ASSERT(tex);

	ImageStateTracking* tracked = ensureTrackedImage(tex);
	tracked->expectedLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	VerifyImageResourceStates();
	if (tex->format == VulkanRenderer::G_DEPTH_FORMAT) 
	{
		VkImageSubresourceRange srr{ VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, tex->mipLevels, 0, tex->layerCount };	
		vkCmdClearDepthStencilImage(m_VkCommandBuffer, tex->image.image, tracked->currentLayout
			, &clear.depthStencil, 1, &srr);
	}
	else 
	{
		VkImageSubresourceRange srr{ VK_IMAGE_ASPECT_COLOR_BIT, 0, tex->mipLevels, 0, tex->layerCount };	
		vkCmdClearColorImage(m_VkCommandBuffer, tex->image.image, tracked->currentLayout
			, &clear.color, 1, &srr);
	}
}

void CommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
	PROFILE_SCOPED();
	BeginRendering(m_renderArea);
	vkCmdDraw(m_VkCommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
	PROFILE_SCOPED();
	BeginRendering(m_renderArea);
	vkCmdDrawIndexed(m_VkCommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CommandList::DrawIndexedIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
	PROFILE_SCOPED();
	BeginRendering(m_renderArea);
	::DrawIndexedIndirect(m_VkCommandBuffer, buffer, offset, drawCount, stride);
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
	PROFILE_SCOPED();
	BeginRendering(m_renderArea);
	vkCmdDraw(m_VkCommandBuffer, 3, 1, 0, 0);
}

void CommandList::Dispatch(uint32_t x, uint32_t y, uint32_t z)
{
	PROFILE_SCOPED();
	VerifyResourceStates();
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

VkCommandBuffer CommandList::getCommandBuffer()
{
	return m_VkCommandBuffer;
}

void CommandList::PrepareDescriptors()
{
	dynamicOffsetCnt = 0;
	setCount = 0;
	firstSet = UINT32_MAX;
	for (size_t i = 0; i < descriptorSets.size(); i++)
	{
		DescriptorSetInfo& descSet = descriptorSets[i];
		if (descSet.expected && descSet.bound == false)
		{
			firstSet = std::min(setCount, firstSet);

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

			sets[setCount] = descSet.descriptor;
			descSet.bound = true;
			setCount++;
		}
	}
}

void CommandList::CommitDescriptors()
{
	PROFILE_SCOPED();

	

	if (setCount == 0)
		return;

	vkCmdBindDescriptorSets(
		m_VkCommandBuffer,
		m_pipelineBindPoint,
		m_pipeLayout,
		firstSet,
		setCount,
		sets.data(),
		dynamicOffsetCnt,
		dynamicOffsetCnt ? dynOffsets.data() : nullptr);
}

void CommandList::DenoteStateChanged()
{
	m_attachmentReady = false;
}

void CommandList::EndIfRendering()
{
	if (m_currentlyRendering == true)
		EndRendering();
}

void CommandList::GetOrBuildPipeline()
{
	if (m_pipeline != VK_NULL_HANDLE) return; // no pipeline, we assume bound
	OO_ASSERT(m_pipelineBindPoint != VK_PIPELINE_BIND_POINT_MAX_ENUM);

	auto& vr = *VulkanRenderer::get();
	size_t hash{ 0 };
	size_t setCount{};
	std::vector<VkDescriptorSetLayout> setLayouts;
	for (size_t i = 0; i < descriptorSets.size(); i++)
	{
		if (descriptorSets[i].expected) {
			oGFX::HashCombine(hash, descriptorSets[i].builder.getHash());
			setCount++;
			setLayouts.push_back(descriptorSets[i].layout);
		}
	}
	oGFX::HashCombine(hash, setCount);
	size_t pipelineLayoutHash = hash;
	{
		auto& map = vr.pipelineLayoutMap;
		auto iter = map.find(pipelineLayoutHash);
		if (iter == map.end()) {
			// does not contain, build;
			VkPipelineLayoutCreateInfo plci = oGFX::vkutils::inits::pipelineLayoutCreateInfo(
				setLayouts.data(), static_cast<uint32_t>(setLayouts.size()));

			printf("Creating pipeline layout hash-0x%zX\n", pipelineLayoutHash);
			VK_CHK(vkCreatePipelineLayout(vr.m_device.logicalDevice, &plci, nullptr, &m_pipeLayout));
			//VK_NAME(m_device.logicalDevice, "Bloom_PSOLayout", play);
			map[pipelineLayoutHash] = m_pipeLayout;
		}
		else {
			m_pipeLayout = iter->second;
		}
	}

	if (m_pipelineBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE) 
	{		
		oGFX::StringHash comp(shadercodes[COMPUTE]);
		oGFX::HashCombine(hash, comp.computedHash);
		{
			auto& map = vr.pipelineMap;
			auto iter = map.find(hash);
			if (iter == map.end()) {
				// does not contain, build;

				printf("Creating compute pipeline hash-0x%zX\n", hash);
				computeCI = oGFX::vkutils::inits::computeCreateInfo(m_pipeLayout);
				computeCI.stage = vr.LoadShader(vr.m_device, shadercodes[COMPUTE], VK_SHADER_STAGE_COMPUTE_BIT);
				VK_CHK(vkCreateComputePipelines(vr.m_device.logicalDevice, VK_NULL_HANDLE, 1, &computeCI, nullptr, &m_pipeline));
				vkDestroyShaderModule(vr.m_device.logicalDevice, computeCI.stage.module, nullptr);
				map[hash] = m_pipeline;
			}
			else {
				m_pipeline = iter->second;
			}
		}
	}
	else 
	{
		// do graphics pipeline
		oGFX::StringHash vert(shadercodes[VERTEX]);
		oGFX::StringHash frag(shadercodes[FRAGMENT]);
		oGFX::HashCombine(hash, vert.computedHash);
		oGFX::HashCombine(hash, frag.computedHash);

		oGFX::HashCombine(hash, inputAssemblyState);
		oGFX::HashCombine(hash, rasterizationState);
		oGFX::HashCombine(hash, colorBlendState);
		oGFX::HashCombine(hash, multisampleState);
		oGFX::HashCombine(hash, viewportState);
		oGFX::HashCombine(hash, depthStencilState);

		{
			auto& map = vr.pipelineMap;
			auto iter = map.find(hash);
			if (iter == map.end()) {
				// does not contain, build;

				printf("Creating graphics pipeline hash-0x%zX\n", hash);
				pipelineCI = oGFX::vkutils::inits::pipelineCreateInfo(m_pipeLayout);

				std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages =
				{
					vr.LoadShader(vr.m_device, shadercodes[VERTEX  ], VK_SHADER_STAGE_VERTEX_BIT),
					vr.LoadShader(vr.m_device, shadercodes[FRAGMENT], VK_SHADER_STAGE_FRAGMENT_BIT)
				};

				pipelineCI.pInputAssemblyState = &inputAssemblyState;
				pipelineCI.pRasterizationState = &rasterizationState;
				pipelineCI.pColorBlendState = &colorBlendState;
				pipelineCI.pMultisampleState = &multisampleState;
				pipelineCI.pViewportState = &viewportState;
				pipelineCI.pDepthStencilState = &depthStencilState;
				pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
				pipelineCI.pStages = shaderStages.data();

				// Need to enable to 
				depthStencilState.stencilTestEnable = VK_TRUE;
				depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
				depthStencilState.back.failOp = VK_STENCIL_OP_REPLACE;
				depthStencilState.back.depthFailOp = VK_STENCIL_OP_REPLACE;
				depthStencilState.back.passOp = VK_STENCIL_OP_REPLACE;
				depthStencilState.back.compareMask = 0xff;
				depthStencilState.back.writeMask = 0xff;
				depthStencilState.back.reference = 1;
				depthStencilState.front = depthStencilState.back;
				// =======================

				pipelineCI.pVertexInputState = &vertexInputCreateInfo;

				// not hashed as assumed not changing
				pipelineCI.pDynamicState = &dynamicState;

				size_t numAttachments = m_highestAttachmentBound + 1;

				std::vector<VkFormat> colourFormats;
				colourFormats.resize(numAttachments);
				for (size_t i = 0; i < numAttachments; i++)
				{
					colourFormats[i] = m_attachmentFormats[i];
				}

				std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;
				blendAttachmentStates.resize(numAttachments);
				for (size_t i = 0; i < numAttachments; i++)
				{
					blendAttachmentStates[i] = oGFX::vkutils::inits::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
				}				

				colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
				colorBlendState.pAttachments = blendAttachmentStates.data();

				VkPipelineRenderingCreateInfo renderingInfo{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
				renderingInfo.viewMask = {};
				renderingInfo.colorAttachmentCount = (uint32_t)numAttachments;
				renderingInfo.pColorAttachmentFormats = colourFormats.data();
				renderingInfo.depthAttachmentFormat = m_depthFormat;
				renderingInfo.stencilAttachmentFormat = m_depthFormat;

				pipelineCI.pNext = &renderingInfo;

				VK_CHK(vkCreateGraphicsPipelines(vr.m_device.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &m_pipeline));

				vkDestroyShaderModule(vr.m_device.logicalDevice, shaderStages[VERTEX  ].module, nullptr);
				vkDestroyShaderModule(vr.m_device.logicalDevice, shaderStages[FRAGMENT].module, nullptr);

				map[hash] = m_pipeline;
			}
			else {
				m_pipeline = iter->second;
			}
		}
	}
	vkCmdBindPipeline(m_VkCommandBuffer, m_pipelineBindPoint, m_pipeline);
}

DescriptorSetInfo& DescriptorSetInfo::BindImage(uint32_t binding, vkutils::Texture* texture, VkDescriptorType type)
{	
	OO_ASSERT(texture);
	BindImage(binding, texture, type, texture->view);
	return *this;
}

DescriptorSetInfo& DescriptorSetInfo::BindImage(uint32_t binding, vkutils::Texture* texture, VkDescriptorType type, VkImageView viewOverride)
{
	VkImageLayout layout = (type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	OO_ASSERT(viewOverride);

	VkDescriptorImageInfo texinfo = oGFX::vkutils::inits::descriptorImageInfo(
		VK_NULL_HANDLE, // no sampler
		viewOverride,
		layout);

	builder.BindImage(binding, &texinfo, type, shaderStage);

	this->m_cmdList->BeginTrackingImage(texture);
	ImageStateTracking* tracked = m_cmdList->getTrackedImage(texture);
	tracked->expectedLayout = layout;

	return *this;
}

DescriptorSetInfo& DescriptorSetInfo::BindImage(uint32_t binding, VkDescriptorImageInfo* imageinfos, VkDescriptorType type, uint32_t count)
{
	OO_ASSERT(imageinfos);	
	builder.BindImage(binding, imageinfos, type, shaderStage, count);		
	
	return *this;
}

DescriptorSetInfo& DescriptorSetInfo::BindSampler(uint32_t binding, VkSampler sampler, VkShaderStageFlags stageFlagsInclude)
{
	OO_ASSERT(sampler != VK_NULL_HANDLE);

	VkDescriptorImageInfo samplerInfo = oGFX::vkutils::inits::descriptorImageInfo(
		sampler, 
		VK_NULL_HANDLE,
		VK_IMAGE_LAYOUT_UNDEFINED);

	builder.BindImage(binding, &samplerInfo, VK_DESCRIPTOR_TYPE_SAMPLER , shaderStage | stageFlagsInclude);
	return *this;
}

DescriptorSetInfo& DescriptorSetInfo::BindBuffer(uint32_t binding, const VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, ResourceUsage access, VkShaderStageFlags stageFlagsInclude)
{
	OO_ASSERT(bufferInfo);

	builder.BindBuffer(binding, bufferInfo, type, shaderStage | stageFlagsInclude);
	this->m_cmdList->BeginTrackingBuffer(bufferInfo->buffer);
	BufferStateTracking* tracked = m_cmdList->getTrackedBuffer(bufferInfo->buffer);
	tracked->expectedAccess = access;

	return *this;
}

DescriptorSetInfo& DescriptorSetInfo::SetDynamicOffset(uint32_t binding, uint32_t offset)
{
	hasDynamicOffset = true;
	dynamicOffset = offset;

	return *this;
}

}
