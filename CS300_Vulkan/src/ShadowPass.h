#pragma once

#include "GfxRenderpass.h"
#include "vulkan/vulkan.h"
#include "imgui.h"
#include "VulkanFramebufferAttachment.h"

#include <array>

struct ShadowPass : public GfxRenderpass
{
	//DECLARE_RENDERPASS_SINGLETON(ShadowPass)

	void Init() override;
	void Draw() override;
	void Shutdown() override;

	void CreatePSO() override;
	
	VulkanFramebufferAttachment att_depth;

	// This is for ImGui
	std::array<ImTextureID, GBufferAttachmentIndex::TOTAL_COLOR_ATTACHMENTS> deferredImg;
	ImTextureID shadowImg;

	VkExtent2D shadowmapSize = { 1024, 1024 };

	VkRenderPass renderpass_Shadow;
	VkFramebuffer framebuffer_Shadow;

	VkPipeline pso_ShadowDefault;

private:
	void SetupRenderpass();
	void SetupFramebuffer();
	void CreatePipeline();
};