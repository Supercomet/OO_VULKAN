#pragma once
#include "GfxRenderpass.h"
#include "vulkan/vulkan.h"
#include "imgui.h"
#include "VulkanFramebufferAttachment.h"
struct DeferredCompositionRenderpass : public GfxRenderpass
{
	DECLARE_RENDERPASS_SINGLETON(DeferredCompositionRenderpass)

	void Init() override;
	void Draw() override;
	void Shutdown() override;

	VkRenderPass compositionPass;
	VkPipelineLayout compositionPipeLayout;
	VkPipeline compositionPipe;
private:
	void CreatePipeline();
};

