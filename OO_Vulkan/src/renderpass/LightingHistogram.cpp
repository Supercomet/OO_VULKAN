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
VkPipeline pso_lightingCDFScan{};

void LightingHistogram::Init()
{
	SetupRenderpass();
	SetupFramebuffer();
	SetupDependencies();
}

void LightingHistogram::CreatePSO()
{
	CreatePipeline();
}

bool LightingHistogram::SetupDependencies()
{
	auto& vr = *VulkanRenderer::get();
	VmaAllocationCreateFlags flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	oGFX::CreateBuffer(vr.m_device.m_allocator, sizeof(HistoStruct)
						, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, flags
						, vr.lightingHistogram);
	
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
	
	rhi::CommandList cmd{ cmdlist, "LightingHistogram"};

	VkDescriptorBufferInfo dbi{};
	dbi.buffer = vr.lightingHistogram.buffer;
	dbi.offset = 0;
	dbi.range = VK_WHOLE_SIZE;

	cmd.BindPSO(pso_LightingHistogram, PSOLayoutDB::histogramPSOLayout,VK_PIPELINE_BIND_POINT_COMPUTE);
	cmd.DescriptorSetBegin(0)
		.BindImage(0, &vr.attachments.lighting_target, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		.BindBuffer(1, &dbi, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

	cmd.Dispatch(vr.attachments.lighting_target.width / 16 + 1, vr.attachments.lighting_target.height / 16 + 1);

	cmd.BindPSO(pso_lightingCDFScan, PSOLayoutDB::histogramPSOLayout,VK_PIPELINE_BIND_POINT_COMPUTE);
	cmd.Dispatch(1);
}

void LightingHistogram::Shutdown()
{
	auto& vr = *VulkanRenderer::get();
	auto& device = vr.m_device.logicalDevice;

	vmaDestroyBuffer(vr.m_device.m_allocator, vr.lightingHistogram.buffer, vr.lightingHistogram.alloc);

	vkDestroyPipelineLayout(device, PSOLayoutDB::histogramPSOLayout, nullptr);
	vr.attachments.shadow_depth.destroy();
	vkDestroyPipeline(device, pso_LightingHistogram, nullptr);
	vkDestroyPipeline(device, pso_lightingCDFScan, nullptr);
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

	VkPipelineLayoutCreateInfo plci = oGFX::vkutils::inits::pipelineLayoutCreateInfo(&SetLayoutDB::compute_histogram,1);
	vkCreatePipelineLayout(m_device.logicalDevice, &plci, nullptr, &PSOLayoutDB::histogramPSOLayout);

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

	
	VkComputePipelineCreateInfo computeCI = oGFX::vkutils::inits::computeCreateInfo(PSOLayoutDB::histogramPSOLayout);
	const char* histoShader=  "Shaders/bin/histogram.comp.spv";
	computeCI.stage = vr.LoadShader(m_device, histoShader, VK_SHADER_STAGE_COMPUTE_BIT);
	
	if (pso_LightingHistogram != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(m_device.logicalDevice, pso_LightingHistogram, nullptr);
	}
	VK_CHK(vkCreateComputePipelines(m_device.logicalDevice, VK_NULL_HANDLE, 1, &computeCI, nullptr, &pso_LightingHistogram));
	VK_NAME(m_device.logicalDevice, "pso_LightingHistogram", pso_LightingHistogram);
	vkDestroyShaderModule(m_device.logicalDevice, computeCI.stage.module, nullptr);

	const char* cdfShader=  "Shaders/bin/cdfscan.comp.spv";
	computeCI.stage = vr.LoadShader(m_device, cdfShader, VK_SHADER_STAGE_COMPUTE_BIT);
	if (pso_lightingCDFScan != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(m_device.logicalDevice, pso_lightingCDFScan, nullptr);
	}
	VK_CHK(vkCreateComputePipelines(m_device.logicalDevice, VK_NULL_HANDLE, 1, &computeCI, nullptr, &pso_lightingCDFScan));
	VK_NAME(m_device.logicalDevice, "pso_lightingCDFScan", pso_lightingCDFScan);
	vkDestroyShaderModule(m_device.logicalDevice, computeCI.stage.module, nullptr);

}

