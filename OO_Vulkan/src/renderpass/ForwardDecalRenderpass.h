#pragma once

#include "GfxRenderpass.h"
#include "vulkan/vulkan.h"
#include "imgui/imgui.h"

#include <array>

struct ForwardDecalRenderpass : public GfxRenderpass
{
    void Init() override;
    void Draw() override;
    void Shutdown() override;

    void CreatePSO() override;

    bool SetupDependencies() { return true; }

    VkRenderPass renderpass_ForwardDecal;
    VkFramebuffer framebuffer_ForwardDecal;
    VkPipeline pso_ForwardDecalDefault;

    
    VkDescriptorSet descriptorSet_ForwardDecal;

private:
    void SetupRenderpass();
    void SetupFramebuffer();
    void CreateDescriptorLayout();
    void CreatePipeline();
    void CreatePipelineLayout();
};
