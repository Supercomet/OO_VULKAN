#pragma once
#include "GfxRenderpass.h"
#include "vulkan/vulkan.h"
#include "imgui.h"
#include "VulkanFramebufferAttachment.h"

struct GBufferRenderPass : public GfxRenderpass
{
	//DECLARE_RENDERPASS_SINGLETON(GBufferRenderPass)

	void Init() override;
	void Draw() override;
	void Shutdown() override;

	void CreatePSO() override;
	
	VulkanFramebufferAttachment att_albedo;
	VulkanFramebufferAttachment att_position;
	VulkanFramebufferAttachment att_normal;
	VulkanFramebufferAttachment att_depth;

	ImTextureID deferredImg[3]{};

	VkRenderPass renderpass_GBuffer;
	VkFramebuffer framebuffer_GBuffer;

	uint64_t uboDynamicAlignment;
	//VkPushConstantRange pushConstantRange;
	VkPipeline pso_GBufferDefault;

private:
	void SetupRenderpass();
	void SetupFramebuffer();
	void CreateDescriptors();
	void CreatePipeline();

};