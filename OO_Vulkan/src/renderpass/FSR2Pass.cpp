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
	FfxInt32x2    renderSize;
	FfxInt32x2    maxRenderSize;
	FfxInt32x2    displaySize;
	FfxInt32x2    inputColorResourceDimensions;
	FfxInt32x2    lumaMipDimensions;
	FfxInt32      lumaMipLevelToUse;
	FfxInt32      frameIndex;

	FfxFloat32x4  deviceToViewDepth;
	FfxFloat32x2  jitterOffset;
	FfxFloat32x2  motionVectorScale;
	FfxFloat32x2  downscaleFactor;
	FfxFloat32x2  motionVectorJitterCancellation;
	FfxFloat32    preExposure;
	FfxFloat32    previousFramePreExposure;
	FfxFloat32    tanHalfFOV;
	FfxFloat32    jitterPhaseCount;
	FfxFloat32    deltaTime;
	FfxFloat32    dynamicResChangeFactor;
	FfxFloat32    viewSpaceToMetersFactor;
};

FSR2_CB_DATA constantBuffer{};

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
	auto& vr = *VulkanRenderer::get();
	constexpr size_t MAX_FRAMES = 2;


	for (size_t i = 0; i < MAX_FRAMES; i++)
	{
		oGFX::CreateBuffer(vr.m_device.m_allocator, sizeof(FSR2_CB_DATA)
			, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
			, vr.FSR2constantBuffer[i]);
		VK_NAME(vr.m_device.logicalDevice, "FSR2 CB", vr.FSR2constantBuffer[i].buffer);
	}


	return true;
}

#define FFX_FSR2_ENABLE_DEPTH_INVERTED true
#define FFX_FSR2_ENABLE_DEPTH_INFINITE true
#define FFX_FSR2_ENABLE_DISPLAY_RESOLUTION_MOTION_VECTORS false

static void setupDeviceDepthToViewSpaceDepthParams(const Camera& cam)
{

	auto& vr = *VulkanRenderer::get();
	VkExtent2D renderSize = {vr.m_swapchain.swapChainExtent.width * vr.renderResolution ,vr.m_swapchain.swapChainExtent.height * vr.renderResolution };

	const bool bInverted = FFX_FSR2_ENABLE_DEPTH_INVERTED;
	const bool bInfinite = FFX_FSR2_ENABLE_DEPTH_INFINITE;

	// make sure it has no impact if near and far plane values are swapped in dispatch params
	// the flags "inverted" and "infinite" will decide what transform to use
	float fMin = std::min(cam.m_znear, cam.m_zfar);
	float fMax = std::max(cam.m_znear, cam.m_zfar);

	if (bInverted) {
		float tmp = fMin;
		fMin = fMax;
		fMax = tmp;
	}

	// a 0 0 0   x
	// 0 b 0 0   y
	// 0 0 c d   z
	// 0 0 e 0   1

	const float fQ = fMax / (fMin - fMax);
	const float d = -1.0f; // for clarity

	const float matrix_elem_c[2][2] = {
		fQ,                     // non reversed, non infinite
		-1.0f - FLT_EPSILON,    // non reversed, infinite
		fQ,                     // reversed, non infinite
		0.0f + FLT_EPSILON      // reversed, infinite
	};

	const float matrix_elem_e[2][2] = {
		fQ * fMin,             // non reversed, non infinite
		-fMin - FLT_EPSILON,    // non reversed, infinite
		fQ * fMin,             // reversed, non infinite
		fMax,                  // reversed, infinite
	};

	constantBuffer.deviceToViewDepth[0] = d * matrix_elem_c[bInverted][bInfinite];
	constantBuffer.deviceToViewDepth[1] = matrix_elem_e[bInverted][bInfinite];

	// revert x and y coords
	const float aspect = renderSize.width / float(renderSize.height);
	const float cotHalfFovY = cosf(0.5f * glm::radians(cam.m_fovDegrees)) / sinf(0.5f * glm::radians(cam.m_fovDegrees));
	const float a = cotHalfFovY / aspect;
	const float b = cotHalfFovY;

	constantBuffer.deviceToViewDepth[2] = (1.0f / a);
	constantBuffer.deviceToViewDepth[3] = (1.0f / b);
}

int32_t ffxFsr2GetJitterPhaseCount(int32_t renderWidth, int32_t displayWidth)
{
	const float basePhaseCount = 8.0f;
	const int32_t jitterPhaseCount = int32_t(basePhaseCount * pow((float(displayWidth) / renderWidth), 2.0f));
	return jitterPhaseCount;
}

// Calculate halton number for index and base.
static float halton(int32_t index, int32_t base)
{
	float f = 1.0f, result = 0.0f;

	for (int32_t currentIndex = index; currentIndex > 0;) {

		f /= (float)base;
		result = result + f * (float)(currentIndex % base);
		currentIndex = (uint32_t)(floorf((float)(currentIndex) / (float)(base)));
	}

	return result;
}

void ffxFsr2GetJitterOffset(float* outX, float* outY, int32_t index, int32_t phaseCount)
{
	OO_ASSERT(outX && outY);
	OO_ASSERT(phaseCount > 0);

	const float x = halton((index % phaseCount) + 1, 2) - 0.5f;
	const float y = halton((index % phaseCount) + 1, 3) - 0.5f;

	*outX = x;
	*outY = y;
}

void SetupConstantBuffer() 
{
	VulkanRenderer& vr = *VulkanRenderer::get();

	Camera& cam = vr.currWorld->cameras[0];
	VkExtent2D resInfo = vr.m_swapchain.swapChainExtent;
	// Increment jitter index for frame
	++vr.m_JitterIndex;

	float         jitterX, jitterY;
	const int32_t jitterPhaseCount = ffxFsr2GetJitterPhaseCount(vr.renderWidth, resInfo.width);
	ffxFsr2GetJitterOffset(&jitterX, &jitterY, vr.m_JitterIndex, jitterPhaseCount);

	// TODO: setup camera to use jittered projection
	// pCamera->SetJitterValues({ -2.f * jitterX / vr.renderWidth, 2.f * jitterY / vr.renderHeight });

	constantBuffer = {};
	constantBuffer.displaySize[0] = resInfo.width;
	constantBuffer.displaySize[1] = resInfo.height;

	constantBuffer.jitterOffset[0] = jitterX;
	constantBuffer.jitterOffset[1] = jitterY;
	constantBuffer.renderSize[0] = vr.renderWidth;
	constantBuffer.renderSize[1] = vr.renderHeight;
	constantBuffer.maxRenderSize[0] = vr.renderWidth;
	constantBuffer.maxRenderSize[1] = vr.renderHeight;

	// or colour .size
	constantBuffer.inputColorResourceDimensions[0] = vr.renderWidth;
	constantBuffer.inputColorResourceDimensions[1] = vr.renderHeight;

	// compute the horizontal FOV for the shader from the vertical one.
	const float aspectRatio = (float)vr.renderWidth / (float)vr.renderHeight;
	const float cameraAngleHorizontal = atan(tan(glm::radians(cam.m_fovDegrees) / 2) * aspectRatio) * 2;
	constantBuffer.tanHalfFOV = tanf(cameraAngleHorizontal * 0.5f);
	constantBuffer.viewSpaceToMetersFactor = 1.0f; // wtf is this

	// compute params to enable device depth to view space depth computation in shader
	setupDeviceDepthToViewSpaceDepthParams(cam);

	// To be updated if resource is larger than the actual image size
	constantBuffer.downscaleFactor[0] = float(vr.renderWidth) / resInfo.width;
	constantBuffer.downscaleFactor[1] = float(vr.renderHeight) / resInfo.height;
	constantBuffer.previousFramePreExposure = constantBuffer.preExposure;
	constantBuffer.preExposure = (constantBuffer.previousFramePreExposure != 0) ? constantBuffer.previousFramePreExposure : 1.0f;

	VkExtent2D renderSize = { vr.renderWidth ,vr.renderHeight};
	// motion vector data
	const uint32_t* motionVectorsTargetSize = (FFX_FSR2_ENABLE_DISPLAY_RESOLUTION_MOTION_VECTORS) ? (uint32_t*)&resInfo : (uint32_t*)&renderSize;

	// constantBuffer.motionVectorScale[0] = (params->motionVectorScale.x / motionVectorsTargetSize[0]);
	// constantBuffer.motionVectorScale[1] = (params->motionVectorScale.y / motionVectorsTargetSize[1]);
	// 
	// // compute jitter cancellation
	// if (context->contextDescription.flags & FFX_FSR2_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION) {
	// 
	// 	constantBuffer.motionVectorJitterCancellation[0] = (context->previousJitterOffset[0] - context->constants.jitterOffset[0]) / motionVectorsTargetSize[0];
	// 	constantBuffer.motionVectorJitterCancellation[1] = (context->previousJitterOffset[1] - context->constants.jitterOffset[1]) / motionVectorsTargetSize[1];
	// 
	// 	context->previousJitterOffset[0] = context->constants.jitterOffset[0];
	// 	context->previousJitterOffset[1] = context->constants.jitterOffset[1];
	// }

	

}

void FSR2Pass::Draw(const VkCommandBuffer cmdlist)
{
	VulkanRenderer& vr = *VulkanRenderer::get();
	uint32_t currFrame = vr.getFrame();
	Window* windowPtr = vr.windowPtr;
	
	PROFILE_GPU_CONTEXT(cmdlist);
	PROFILE_GPU_EVENT("FSR2");
	rhi::CommandList cmd{ cmdlist, "FSR2",{1,0,0,0.5} };
	lastCmd = cmdlist;

	return;

	SetupConstantBuffer();
	
	//  FSR2_BIND_SRV_INPUT_COLOR                     0
	// 
	//  FSR2_BIND_UAV_SPD_GLOBAL_ATOMIC            2001
	//  FSR2_BIND_UAV_EXPOSURE_MIP_LUMA_CHANGE     2002
	//  FSR2_BIND_UAV_EXPOSURE_MIP_5               2003
	//  FSR2_BIND_UAV_AUTO_EXPOSURE                2004
	// 
	//  FSR2_BIND_CB_FSR2                          3000
	//  FSR2_BIND_CB_SPD                           3001
	cmd.BindPSO(pso_fsr2[FSR2::COMPUTE_LUMINANCE_PYRAMID], PSOLayoutDB::fsr2_PSOLayouts[FSR2::COMPUTE_LUMINANCE_PYRAMID], VK_PIPELINE_BIND_POINT_COMPUTE);
	cmd.DescriptorSetBegin(0)
		.BindImage(0, &vr.attachments.lighting_target, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)

		.BindSampler(1000, GfxSamplerManager::GetSampler_PointClamp()) // point clamp
		.BindSampler(1001, GfxSamplerManager::GetSampler_LinearClamp()) // linear clamp

		.BindImage(2001, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		.BindImage(2002, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		.BindImage(2003, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		.BindImage(2004, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)

		.BindBuffer(3000, vr.FSR2constantBuffer[currFrame].getBufferInfoPtr(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		.BindBuffer(3001, nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
}

void FSR2Pass::Shutdown()
{
	auto& vr = *VulkanRenderer::get();
	auto& device = vr.m_device.logicalDevice;
	
	constexpr size_t MAX_FRAMES = 2;
	for (size_t i = 0; i < MAX_FRAMES; i++)
	{
		vmaDestroyBuffer(vr.m_device.m_allocator, vr.FSR2constantBuffer[i].buffer, vr.FSR2constantBuffer[i].alloc);
	}

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
