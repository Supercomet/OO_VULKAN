/************************************************************************************//*!
\file           FSR2Pass.cpp
\project        Ouroboros
\author         Jamie Kong, j.kong, 390004720 | code contribution (100%)
\par            email: j.kong\@digipen.edu
\date            Nov 8, 2022
\brief              Defines a SSAO pass

Copyright (C) 2022 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*//*************************************************************************************/
#include "GfxRenderpass.h"

#include "VulkanRenderer.h"
#include "Window.h"
#include "VulkanUtils.h"

#include <array>
#include <random>

// FFX defines
#ifndef FFX_CPU
#define FFX_CPU
#endif // !FFX_CPU
#ifndef FFX_GLSL
#define FFX_GLSL
#endif // !FFX_GLSL

//FFX values
#include "../shaders/shared_structs.h"
#include "../shaders/fidelity/include/FidelityFX/gpu/ffx_common_types.h"

// must match FSR2 enum
static const char* fsr_shaders[]{
	"Shaders/bin/ffx_fsr2_tcr_autogen_pass.glsl.spv",
	"Shaders/bin/ffx_fsr2_autogen_reactive_pass.glsl.spv",
	"Shaders/bin/ffx_fsr2_compute_luminance_pyramid_pass.glsl.spv",
	"Shaders/bin/ffx_fsr2_reconstruct_previous_depth_pass.glsl.spv",
	"Shaders/bin/ffx_fsr2_depth_clip_pass.glsl.spv",
	"Shaders/bin/ffx_fsr2_lock_pass.glsl.spv",
	"Shaders/bin/ffx_fsr2_accumulate_pass.glsl.spv",
	"Shaders/bin/ffx_fsr2_rcas_pass.glsl.spv",
};

static const char* fsr_shaders_names[]{
	"fsr2_tcr_autogen",
	"fsr2_autogen_reactive",
	"fsr2_compute_luminance_pyramid",
	"fsr2_reconstruct_previous_depth",
	"fsr2_depth_clip",
	"fsr2_lock",
	"fsr2_accumulate",
	"fsr2_rcas",
};

#pragma optimize("" off)
struct FSR2_CB_DATA {
	FfxInt32x2    iRenderSize;
	FfxInt32x2    iMaxRenderSize;
	FfxInt32x2    iDisplaySize;
	FfxInt32x2    iInputColorResourceDimensions;
	FfxInt32x2    iLumaMipDimensions;
	FfxInt32      iLumaMipLevelToUse;
	FfxInt32      iFrameIndex;

	FfxFloat32x4  fDeviceToViewDepth;
	FfxFloat32x2  fJitter;
	FfxFloat32x2  fMotionVectorScale;
	FfxFloat32x2  fDownscaleFactor;
	FfxFloat32x2  fMotionVectorJitterCancellation;
	FfxFloat32    fPreExposure;
	FfxFloat32    fPreviousFramePreExposure;
	FfxFloat32    fTanHalfFOV;
	FfxFloat32    fJitterSequenceLength;
	FfxFloat32    fDeltaTime;
	FfxFloat32    fDynamicResChangeFactor;
	FfxFloat32    fViewSpaceToMetersFactor;
};

struct FSR2Pass : public GfxRenderpass
{
	//DECLARE_RENDERPASS_SINGLETON(FSR2Pass)

	void Init() override;
	void Draw(const VkCommandBuffer cmdlist) override;
	void Shutdown() override;

	bool SetupDependencies() override;

	void CreatePSO() override;
	void CreatePipelineLayout();
	void CreateDescriptors();

private:

	void SetupRenderpass();
	void CreatePipeline();

};

DECLARE_RENDERPASS(FSR2Pass);

VkPipeline pso_fsr2_luminance_pyramid{};

VkPipeline pso_fsr2[FSR2::MAX_SIZE]{};

void FSR2Pass::Init()
{
	auto& vr = *VulkanRenderer::get();
	auto swapchainext = vr.m_swapchain.swapChainExtent;



	auto cmd = vr.GetCommandBuffer();
	vr.SubmitSingleCommandAndWait(cmd);
	
	SetupRenderpass();

	CreateDescriptors();
	CreatePipelineLayout();
	CreatePSO();

}

void FSR2Pass::CreatePSO()
{	
	CreatePipeline();
}

bool FSR2Pass::SetupDependencies()
{
	return true;
}

void FSR2Pass::Draw(const VkCommandBuffer cmdlist)
{
	auto& vr = *VulkanRenderer::get();
	auto currFrame = vr.getFrame();
	auto* windowPtr = vr.windowPtr;

	PROFILE_GPU_CONTEXT(cmdlist);
	PROFILE_GPU_EVENT("FSR2");
	rhi::CommandList cmd{ cmdlist, "FSR2",{1,0,0,0.5} };
	// cmd.BindPSO(pso_bloom_bright, PSOLayoutDB::doubleImageStoreLayout, VK_PIPELINE_BIND_POINT_COMPUTE);

}

void FSR2Pass::Shutdown()
{
	auto& vr = *VulkanRenderer::get();
	auto& device = vr.m_device.logicalDevice;
	
	for (size_t i = 0; i < FSR2::MAX_SIZE; i++)
	{
		vkDestroyPipelineLayout(device, PSOLayoutDB::fsr2_PSOLayouts[i], nullptr);
		vkDestroyPipeline(device, pso_fsr2[i], nullptr);
	}
}

void FSR2Pass::CreateDescriptors()
{

	auto& vr = *VulkanRenderer::get();
	auto& target = vr.renderTargets[vr.renderTargetInUseID].texture;
	auto currFrame = vr.getFrame();


	// FSR2_BIND_SRV_INPUT_OPAQUE_ONLY                     0
	// FSR2_BIND_SRV_INPUT_COLOR                           1
	// FSR2_BIND_SRV_INPUT_MOTION_VECTORS                  2
	// FSR2_BIND_SRV_PREV_PRE_ALPHA_COLOR                  3
	// FSR2_BIND_SRV_PREV_POST_ALPHA_COLOR                 4
	// FSR2_BIND_SRV_REACTIVE_MASK                         5
	// FSR2_BIND_SRV_TRANSPARENCY_AND_COMPOSITION_MASK     6
	//
	// FSR2_BIND_UAV_AUTOREACTIVE                       2007
	// FSR2_BIND_UAV_AUTOCOMPOSITION                    2008
	// FSR2_BIND_UAV_PREV_PRE_ALPHA_COLOR               2009
	// FSR2_BIND_UAV_PREV_POST_ALPHA_COLOR              2010
	//
	// FSR2_BIND_CB_FSR2								 3000
	// FSR2_BIND_CB_AUTOREACTIVE                        3001
	DescriptorBuilder::Begin()
		.BindImage(0, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(1, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(2, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(3, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(4, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(5, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(6, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)

		.BindImage(1000, nullptr, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // point clamp
		.BindImage(1001, nullptr, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // linear clamp

		.BindImage(2007, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(2008, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(2009, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(2010, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)

		.BindBuffer(3000, nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindBuffer(3001, nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.BuildLayout(SetLayoutDB::compute_fsr2[FSR2::TCR_AUTOGEN]);


	// FSR2_BIND_SRV_INPUT_OPAQUE_ONLY                     0
	// FSR2_BIND_SRV_INPUT_COLOR                           1
	//
	// FSR2_BIND_UAV_AUTOREACTIVE                       2002
	//
	// FSR2_BIND_CB_REACTIVE                            3000
	// FSR2_BIND_CB_FSR2                                3001
	DescriptorBuilder::Begin()
		.BindImage(0, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(1, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)

		.BindImage(1000, nullptr, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // point clamp
		.BindImage(1001, nullptr, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // linear clamp

		.BindImage(2002, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)

		.BindBuffer(3000, nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindBuffer(3001, nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.BuildLayout(SetLayoutDB::compute_fsr2[FSR2::AUTOGEN_REACTIVE]);


	//  FSR2_BIND_SRV_INPUT_COLOR                     0
	// 
	//  FSR2_BIND_UAV_SPD_GLOBAL_ATOMIC            2001
	//  FSR2_BIND_UAV_EXPOSURE_MIP_LUMA_CHANGE     2002
	//  FSR2_BIND_UAV_EXPOSURE_MIP_5               2003
	//  FSR2_BIND_UAV_AUTO_EXPOSURE                2004
	// 
	//  FSR2_BIND_CB_FSR2                          3000
	//  FSR2_BIND_CB_SPD                           3001
	DescriptorBuilder::Begin()
		.BindImage(0, nullptr , VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)

		.BindImage(1000, nullptr , VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // point clamp
		.BindImage(1001, nullptr , VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // linear clamp

		.BindImage(2001, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(2002, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(2003, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(2004, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)

		.BindBuffer(3000, nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindBuffer(3001, nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.BuildLayout(SetLayoutDB::compute_fsr2[FSR2::COMPUTE_LUMINANCE_PYRAMID]);

	// FSR2_BIND_SRV_INPUT_MOTION_VECTORS                  0
	// FSR2_BIND_SRV_INPUT_DEPTH                           1
	// FSR2_BIND_SRV_INPUT_COLOR                           2
	// FSR2_BIND_SRV_INPUT_EXPOSURE                        3
	// FSR2_BIND_SRV_LUMA_HISTORY                          4
	// 
	// FSR2_BIND_UAV_RECONSTRUCTED_PREV_NEAREST_DEPTH   2005
	// FSR2_BIND_UAV_DILATED_MOTION_VECTORS             2006
	// FSR2_BIND_UAV_DILATED_DEPTH                      2007
	// FSR2_BIND_UAV_PREPARED_INPUT_COLOR               2008
	// FSR2_BIND_UAV_LUMA_HISTORY                       2009
	// FSR2_BIND_UAV_LUMA_INSTABILITY                   2010
	// FSR2_BIND_UAV_LOCK_INPUT_LUMA                    2011
	// 
	// FSR2_BIND_CB_FSR2                                3000
	DescriptorBuilder::Begin()
		.BindImage(0, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(1, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(2, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(3, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(4, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)

		.BindImage(1000, nullptr, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // point clamp
		.BindImage(1001, nullptr, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // linear clamp

		.BindImage(2005, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(2006, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(2007, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(2008, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(2009, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(2010, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(2011, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)

		.BindBuffer(3000, nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.BuildLayout(SetLayoutDB::compute_fsr2[FSR2::RECONSTRUCT_PREVIOUS_DEPTH]);

	// FSR2_BIND_SRV_RECONSTRUCTED_PREV_NEAREST_DEPTH      0
	// FSR2_BIND_SRV_DILATED_MOTION_VECTORS                1
	// FSR2_BIND_SRV_DILATED_DEPTH                         2
	// FSR2_BIND_SRV_REACTIVE_MASK                         3
	// FSR2_BIND_SRV_TRANSPARENCY_AND_COMPOSITION_MASK     4
	// FSR2_BIND_SRV_PREPARED_INPUT_COLOR                  5
	// FSR2_BIND_SRV_PREVIOUS_DILATED_MOTION_VECTORS       6
	// FSR2_BIND_SRV_INPUT_MOTION_VECTORS                  7
	// FSR2_BIND_SRV_INPUT_COLOR                           8
	// FSR2_BIND_SRV_INPUT_DEPTH                           9
	// FSR2_BIND_SRV_INPUT_EXPOSURE                        10
	//
	// FSR2_BIND_UAV_DEPTH_CLIP                          2011
	// FSR2_BIND_UAV_DILATED_REACTIVE_MASKS              2012
	// FSR2_BIND_UAV_PREPARED_INPUT_COLOR                2013
	//
	// FSR2_BIND_CB_FSR2                                 3000
	DescriptorBuilder::Begin()
		.BindImage(0, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(1, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(2, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(3, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(4, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(5, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(6, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(7, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(8, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(9, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(10, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)

		.BindImage(1000, nullptr, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // point clamp
		.BindImage(1001, nullptr, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // linear clamp

		.BindImage(2011, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(2012, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(2013, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)

		.BindBuffer(3000, nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.BuildLayout(SetLayoutDB::compute_fsr2[FSR2::DEPTH_CLIP]);


	// FSR2_BIND_SRV_LOCK_INPUT_LUMA                       0
	//
	// FSR2_BIND_UAV_NEW_LOCKS                          2001
	// FSR2_BIND_UAV_RECONSTRUCTED_PREV_NEAREST_DEPTH   2002
	//
	// FSR2_BIND_CB_FSR2                                3000
	DescriptorBuilder::Begin()
		.BindImage(0, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)

		.BindImage(1000, nullptr, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // point clamp
		.BindImage(1001, nullptr, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // linear clamp

		.BindImage(2001, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(2002, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)

		.BindBuffer(3000, nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.BuildLayout(SetLayoutDB::compute_fsr2[FSR2::LOCK]);
	
	//  FSR2_BIND_SRV_INPUT_EXPOSURE                         0
	//  FSR2_BIND_SRV_DILATED_REACTIVE_MASKS                 1
	///	#if FFX_FSR2_OPTION_LOW_RESOLUTION_MOTION_VECTORS
	//		FSR2_BIND_SRV_DILATED_MOTION_VECTORS             2
	///	#else
	//		FSR2_BIND_SRV_INPUT_MOTION_VECTORS               2
	///	#endif
	//  FSR2_BIND_SRV_INTERNAL_UPSCALED                      3
	//  FSR2_BIND_SRV_LOCK_STATUS                            4
	//  FSR2_BIND_SRV_INPUT_DEPTH_CLIP                       5
	//  FSR2_BIND_SRV_PREPARED_INPUT_COLOR                   6
	//  FSR2_BIND_SRV_LUMA_INSTABILITY                       7
	//  FSR2_BIND_SRV_LANCZOS_LUT                            8
	//  FSR2_BIND_SRV_UPSCALE_MAXIMUM_BIAS_LUT               9
	//  FSR2_BIND_SRV_SCENE_LUMINANCE_MIPS                   10
	//  FSR2_BIND_SRV_AUTO_EXPOSURE                          11
	//  FSR2_BIND_SRV_LUMA_HISTORY                           12
	// 
	//  FSR2_BIND_UAV_INTERNAL_UPSCALED                      2013
	//  FSR2_BIND_UAV_LOCK_STATUS                            2014
	//  FSR2_BIND_UAV_UPSCALED_OUTPUT                        2015
	//  FSR2_BIND_UAV_NEW_LOCKS                              2016
	//  FSR2_BIND_UAV_LUMA_HISTORY                           2017
	// 
	//  FSR2_BIND_CB_FSR2                                    3000
	DescriptorBuilder::Begin()
		.BindImage(0, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(1, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(2, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(3, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(4, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(5, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(6, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(7, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(8, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(9, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(10, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(11, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(12, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)

		.BindImage(1000, nullptr, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // point clamp
		.BindImage(1001, nullptr, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // linear clamp

		.BindImage(2013, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(2014, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(2015, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(2016, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(2017, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)

		.BindBuffer(3000, nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.BuildLayout(SetLayoutDB::compute_fsr2[FSR2::ACCUMULATE]);


	// FSR2_BIND_SRV_INPUT_EXPOSURE        0
	// FSR2_BIND_SRV_RCAS_INPUT            1
	//
	// FSR2_BIND_UAV_UPSCALED_OUTPUT    2002
	//
	// FSR2_BIND_CB_FSR2                3000
	// FSR2_BIND_CB_RCAS                3001
	DescriptorBuilder::Begin()
		.BindImage(0, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(1, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)

		.BindImage(1000, nullptr, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // point clamp
		.BindImage(1001, nullptr, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // linear clamp

		.BindImage(2002, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)

		.BindBuffer(3000, nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindBuffer(3001, nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.BuildLayout(SetLayoutDB::compute_fsr2[FSR2::RCAS]);
	
	
}

void FSR2Pass::CreatePipelineLayout()
{
	auto& vr = *VulkanRenderer::get();
	auto& m_device = vr.m_device;

	// setup all layouts
	for (size_t i = 0; i < FSR2::MAX_SIZE; i++)
	{
		std::vector<VkDescriptorSetLayout> setLayouts
		{
			SetLayoutDB::compute_fsr2[i],
		};

		VkPipelineLayoutCreateInfo plci = oGFX::vkutils::inits::pipelineLayoutCreateInfo(
			setLayouts.data(), static_cast<uint32_t>(setLayouts.size()));

		VkPushConstantRange pushConstantRange{ VK_SHADER_STAGE_ALL, 0, 128 };
		plci.pushConstantRangeCount = 1;
		plci.pPushConstantRanges = &pushConstantRange;
		std::string name(fsr_shaders_names[i]);
		name += "_PSOLayout";
		VK_CHK(vkCreatePipelineLayout(m_device.logicalDevice, &plci, nullptr, &PSOLayoutDB::fsr2_PSOLayouts[i]));
		VK_NAME(m_device.logicalDevice, name.c_str(), PSOLayoutDB::fsr2_PSOLayouts[i]);
	}
}


void FSR2Pass::SetupRenderpass()
{
	auto& vr = *VulkanRenderer::get();
}

void FSR2Pass::CreatePipeline()
{
	auto& vr = *VulkanRenderer::get();
	auto& m_device = vr.m_device;


	VkComputePipelineCreateInfo computeCI;

	for (size_t i = 0; i < FSR2::MAX_SIZE; i++)
	{
		VkPipeline& pipe = pso_fsr2[i];
		if (pipe != VK_NULL_HANDLE) 
		{
			vkDestroyPipeline(m_device.logicalDevice, pipe, nullptr);
		}
		const char* shader = fsr_shaders[i];
		computeCI = oGFX::vkutils::inits::computeCreateInfo(PSOLayoutDB::fsr2_PSOLayouts[i]);
		computeCI.stage = vr.LoadShader(m_device, shader, VK_SHADER_STAGE_COMPUTE_BIT);
		VK_CHK(vkCreateComputePipelines(m_device.logicalDevice, VK_NULL_HANDLE, 1, &computeCI, nullptr, &pipe));
		std::string name(fsr_shaders_names[i]);
		name += "_PSO";		
		VK_NAME(m_device.logicalDevice, name.c_str(), &pipe);
		vkDestroyShaderModule(m_device.logicalDevice, computeCI.stage.module, nullptr); // destroy shader
	}
	
	
}
