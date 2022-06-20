#pragma once
#include "GfxRenderpass.h"
#include "vulkan/vulkan.h"
#include "imgui.h"
#include "VulkanFramebufferAttachment.h"

struct GBufferRenderPass : public GfxRenderpass
{
	DECLARE_RENDERPASS_SINGLETON(GBufferRenderPass)

	void Init() override;
	void Draw() override;
	void Shutdown() override;

	ImTextureID deferredImg[4]{};
	VulkanFramebufferAttachment att_albedo;
	VulkanFramebufferAttachment att_position;
	VulkanFramebufferAttachment att_normal;
	VulkanFramebufferAttachment att_depth;

	VkRenderPass deferredPass;
	VkFramebuffer deferredFB;
	VkSampler deferredSampler;

	uint64_t uboDynamicAlignment;
	VkPushConstantRange pushConstantRange;
	VkPipeline deferredPipe;


private:
	void SetupRenderpass();
	void SetupFramebuffer();
	void CreateSampler();
	void CreateDescriptors();
	void CreatePipeline();

};