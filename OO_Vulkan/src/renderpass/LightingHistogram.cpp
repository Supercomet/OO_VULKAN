/************************************************************************************//*!
\file           LightingHistogram.cpp
\project        Ouroboros
\author         Jamie Kong, j.kong, 390004720 | code contribution (100%)
\par            email: j.kong\@digipen.edu
\date           Oct 02, 2022
\brief              Defines a LightingHistogram

Copyright (C) 2022 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*//*************************************************************************************/
#include "GfxRenderpass.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_vulkan.h"

#include "Window.h"
#include "VulkanRenderer.h"
#include "VulkanUtils.h"
#include "VulkanTexture.h"
#include "FramebufferCache.h"
#include "FramebufferBuilder.h"
#include <iostream>
#include "DebugDraw.h"

#include "../shaders/shared_structs.h"
#include "MathCommon.h"

#include <array>


struct LightingHistogram : public GfxRenderpass
{
	//DECLARE_RENDERPASS_SINGLETON(LightingHistogram)

	void Init() override;
	void Draw(const VkCommandBuffer cmdlist) override;
	void Shutdown() override;

	bool SetupDependencies() override;
	void CreatePSO() override;


private:
	void SetupRenderpass();
	void SetupFramebuffer();
	void CreatePipeline();
};


DECLARE_RENDERPASS(LightingHistogram);


VkPipeline pso_LightingHistogram{};

void LightingHistogram::Init()
{
	SetupRenderpass();
	SetupFramebuffer();
}

void LightingHistogram::CreatePSO()
{
	CreatePipeline();
}

bool LightingHistogram::SetupDependencies()
{
	// TODO: If shadows are disabled, return false.

	// READ: Scene data SSBO
	// READ: Instancing Data
	// WRITE: Shadow Depth Map
	// etc

	return true;
}

void LightingHistogram::Draw(const VkCommandBuffer cmdlist)
{
	auto& vr = *VulkanRenderer::get();

	
	auto& device = vr.m_device;
	auto& swapchain = vr.m_swapchain;
	//auto& commandBuffers = vr.commandBuffers;
	auto currFrame = vr.getFrame();
	auto* windowPtr = vr.windowPtr;

    PROFILE_GPU_CONTEXT(cmdlist);
    PROFILE_GPU_EVENT("LightingHistogram");
	
	const float vpHeight = (float)depth.height;
	const float vpWidth = (float)depth.width;
	rhi::CommandList cmd{ cmdlist, "LightingHistogram"};


	cmd.BindPSO(pso_LightingHistogram, PSOLayoutDB::defaultPSOLayout);

}

void LightingHistogram::Shutdown()
{
	auto& vr = *VulkanRenderer::get();
	auto& device = vr.m_device.logicalDevice;

	vr.attachments.shadow_depth.destroy();
	vkDestroyPipeline(device, pso_LightingHistogram, nullptr);
}

void LightingHistogram::SetupRenderpass()
{
	auto& vr = *VulkanRenderer::get();
	auto& m_device = vr.m_device;
	auto& m_swapchain = vr.m_swapchain;

	VkDescriptorImageInfo dummy{};
	VkDescriptorBufferInfo dummybuf{};

	DescriptorBuilder::Begin()
		.BindImage(0, &dummy, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindBuffer(1, &dummybuf, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.BuildLayout(SetLayoutDB::compute_histogram);

}

void LightingHistogram::SetupFramebuffer()
{
	auto& vr = *VulkanRenderer::get();
	auto& m_device = vr.m_device;
}

void LightingHistogram::CreatePipeline()
{
	auto& vr = *VulkanRenderer::get();
	auto& m_device = vr.m_device;

	
	const char* histoShader=  "Shaders/bin/LightingHistogram.comp.spv";
	VkComputePipelineCreateInfo computeCI = oGFX::vkutils::inits::computeCreateInfo(PSOLayoutDB::histogramPSOLayout);
	computeCI.stage = vr.LoadShader(m_device, histoShader, VK_SHADER_STAGE_COMPUTE_BIT);
	
	if (pso_LightingHistogram != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(m_device.logicalDevice, pso_LightingHistogram, nullptr);
	}
	VK_CHK(vkCreateComputePipelines(m_device.logicalDevice, VK_NULL_HANDLE, 1, &computeCI, nullptr, &pso_LightingHistogram));
	VK_NAME(m_device.logicalDevice, "pso_LightingHistogram", pso_LightingHistogram);
	vkDestroyShaderModule(m_device.logicalDevice, computeCI.stage.module, nullptr);

}

