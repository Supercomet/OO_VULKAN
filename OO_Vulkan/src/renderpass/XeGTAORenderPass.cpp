/************************************************************************************//*!
\file           XeGTAORenderPass.cpp
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

#include "XeGTAOHelper.h"

#include <array>
#include <random>



// must match FSR2 enum
static const char* xegtao_shaders[]{
	"Shaders/bin/xegtao_prefilterDepths.comp.spv"
	,"Shaders/bin/xegtao_main.comp.spv"
	,"Shaders/bin/XeGTAO_denoise.comp.spv"
	,"Shaders/bin/XeGTAO_denoiseLast.comp.spv"
	,"Shaders/bin/XeGTAO_genNorms.comp.spv"
,"max_Str"
};

static const char* xegtao_shaders_names[]{
	"xegtao_prefilterDepths"
	,"xegtao_main"
	,"XeGTAO_denoise"
	,"XeGTAO_denoiseLast"
	,"XeGTAO_genNorms"
	,"max_Str"
};

struct XeGTAORenderPass : public GfxRenderpass
{
	//DECLARE_RENDERPASS_SINGLETON(XeGTAORenderPass)
	XeGTAORenderPass(const char* _name) : GfxRenderpass{ _name } {}

	void Init() override;
	void Draw(const VkCommandBuffer cmdlist) override;
	void Shutdown() override;

	void InitRandomFactors();

	bool SetupDependencies(RenderGraph& builder) override;

	void CreatePSO() override;
	void CreatePipelineLayout();
	void CreateDescriptors();

	void UpdateConstants(XeGTAO::GTAOConstants& consts, XeGTAO::GTAOSettings& settings);

private:
	void SetupRenderpass();
	void CreatePipeline();

};

DECLARE_RENDERPASS(XeGTAORenderPass);

//VkPushConstantRange pushConstantRange;
VkPipeline pso_xegtao[(uint8_t)XEGTAO::MAX_SIZE]{};

vkutils::Texture2D normalA;
vkutils::Texture2D normalB;

void XeGTAORenderPass::Init()
{
	auto& vr = *VulkanRenderer::get();
	auto swapchainext = vr.m_swapchain.swapChainExtent;


	constexpr size_t MAX_FRAMES = 2;
	for (size_t i = 0; i < MAX_FRAMES; i++)
	{
		oGFX::CreateBuffer(vr.m_device.m_allocator, sizeof(XeGTAO::GTAOConstants)
			, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
			, vr.XeGTAOconstants[i]);
		VK_NAME(vr.m_device.logicalDevice, "XeGTAOConstants CB", vr.XeGTAOconstants[i].buffer);
	}


	InitRandomFactors();

	float fullResolution = 1.0f;
	VkFormat AOTermFormat = VK_FORMAT_R8_UINT;
	//VkFormat AOTermFormat = VK_FORMAT_R32_UINT;

	vr.attachments.xegtao_workingDepths.name = "xegtao_workingDepths";
	vr.attachments.xegtao_workingDepths.forFrameBuffer(&vr.m_device, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT,
		swapchainext.width, swapchainext.height, true, fullResolution, XE_GTAO_DEPTH_MIP_LEVELS, VK_IMAGE_LAYOUT_GENERAL);
	vr.fbCache.RegisterFramebuffer(vr.attachments.xegtao_workingDepths);

	vr.attachments.xegtao_genNormals.name = "xegtao_genNormals";
	vr.attachments.xegtao_genNormals.forFrameBuffer(&vr.m_device, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT,
		swapchainext.width, swapchainext.height, true, fullResolution, 1, VK_IMAGE_LAYOUT_GENERAL);
	vr.fbCache.RegisterFramebuffer(vr.attachments.xegtao_genNormals);

	normalA.name = "xegtao_genNormalsTestA";
	normalA.forFrameBuffer(&vr.m_device, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT,
		swapchainext.width, swapchainext.height, true, fullResolution, 1, VK_IMAGE_LAYOUT_GENERAL);
	vr.fbCache.RegisterFramebuffer(normalA);

	normalB.name = "xegtao_genNormalsTestB";
	normalB.forFrameBuffer(&vr.m_device, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT,
		swapchainext.width, swapchainext.height, true, fullResolution, 1, VK_IMAGE_LAYOUT_GENERAL);
	vr.fbCache.RegisterFramebuffer(normalB);

	vr.attachments.xegtao_workingEdges.name = "xegtao_workingEdges";
	vr.attachments.xegtao_workingEdges.forFrameBuffer(&vr.m_device, VK_FORMAT_R8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT,
		swapchainext.width, swapchainext.height, true, fullResolution, 1, VK_IMAGE_LAYOUT_GENERAL);
	vr.fbCache.RegisterFramebuffer(vr.attachments.xegtao_workingEdges);

	vr.attachments.xegtao_workingAOTerm.name = "xegtao_workingAOTerm";
	vr.attachments.xegtao_workingAOTerm.forFrameBuffer(&vr.m_device, AOTermFormat, VK_IMAGE_USAGE_STORAGE_BIT,
		swapchainext.width, swapchainext.height, true, fullResolution, 1, VK_IMAGE_LAYOUT_GENERAL);
	vr.fbCache.RegisterFramebuffer(vr.attachments.xegtao_workingAOTerm);

	vr.attachments.xegtao_workingAOTermPong.name = "xegtao_workingAOTermPong";
	vr.attachments.xegtao_workingAOTermPong.forFrameBuffer(&vr.m_device, AOTermFormat, VK_IMAGE_USAGE_STORAGE_BIT,
		swapchainext.width, swapchainext.height, true, fullResolution, 1, VK_IMAGE_LAYOUT_GENERAL);
	vr.fbCache.RegisterFramebuffer(vr.attachments.xegtao_workingAOTermPong);

	auto cmd = vr.GetCommandBuffer();	
	vkutils::SetImageInitialState(cmd, vr.attachments.xegtao_workingDepths);
	vkutils::SetImageInitialState(cmd, vr.attachments.xegtao_genNormals);
	vkutils::SetImageInitialState(cmd, normalA);
	vkutils::SetImageInitialState(cmd, normalB);
	vkutils::SetImageInitialState(cmd, vr.attachments.xegtao_workingAOTerm);
	vkutils::SetImageInitialState(cmd, vr.attachments.xegtao_workingAOTermPong);
	vkutils::SetImageInitialState(cmd, vr.attachments.xegtao_workingEdges);
	vr.SubmitSingleCommandAndWait(cmd);

	SetupRenderpass();
}

OO_OPTIMIZE_OFF
void XeGTAORenderPass::UpdateConstants(XeGTAO::GTAOConstants& consts, XeGTAO::GTAOSettings& settings)
{
	VulkanRenderer& vr = *VulkanRenderer::get();
	size_t frameCount = vr.currentFrame;
	size_t currFrame = vr.getFrame();

	Camera& cam = vr.currWorld->cameras[0];
	VkExtent2D resInfo = { vr.renderWidth,vr.renderHeight };
	glm::mat4 projMatrix = cam.matrices.perspectiveJittered;
	bool rowMajor = true;

	consts.ViewportSize = { (int32_t)resInfo.width,(int32_t)resInfo.height };
	consts.ViewportPixelSize = { 1.0f / (float)consts.ViewportSize.x, 1.0f / (float)consts.ViewportSize.y };
	
	consts.View = cam.matrices.view;

	float depthLinearizeMul = (rowMajor) ? (-projMatrix[3][2]) : (-projMatrix[2][3]);     // float depthLinearizeMul = ( clipFar * clipNear ) / ( clipFar - clipNear );
	float depthLinearizeAdd = (rowMajor) ? ( projMatrix[2][2]) : ( projMatrix[2][2]);     // float depthLinearizeAdd = clipFar / ( clipFar - clipNear );
	
	// correct the handedness issue. need to make sure this below is correct, but I think it is.
	if (depthLinearizeMul * depthLinearizeAdd < 0)
		depthLinearizeAdd = -depthLinearizeAdd;
	consts.DepthUnpackConsts = { depthLinearizeMul, depthLinearizeAdd };
	
	float tanHalfFOVY = 1.0f / ((rowMajor) ? (projMatrix[1 ][ 1]) : (projMatrix[1 ][1 ]));    // = tanf( drawContext.Camera.GetYFOV( ) * 0.5f );
	float tanHalfFOVX = 1.0F / ((rowMajor) ? (projMatrix[0 ][ 0]) : (projMatrix[0 ][0 ]));    // = tanHalfFOVY * drawContext.Camera.GetAspect( );
	consts.CameraTanHalfFOV = { tanHalfFOVX, tanHalfFOVY };
	
	consts.NDCToViewMul = { consts.CameraTanHalfFOV.x * 2.0f, consts.CameraTanHalfFOV.y * -2.0f };
	consts.NDCToViewAdd = { consts.CameraTanHalfFOV.x * -1.0f, consts.CameraTanHalfFOV.y * 1.0f };
	
	consts.NDCToViewMul_x_PixelSize = { consts.NDCToViewMul.x * consts.ViewportPixelSize.x, consts.NDCToViewMul.y * consts.ViewportPixelSize.y };
	
	consts.EffectRadius = settings.Radius;
	
	consts.EffectFalloffRange = settings.FalloffRange;
	consts.DenoiseBlurBeta = (settings.DenoisePasses == 0) ? (1e4f) : (1.2f);    // high value disables denoise - more elegant & correct way would be do set all edges to 0
	
	consts.RadiusMultiplier = settings.RadiusMultiplier;
	consts.SampleDistributionPower = settings.SampleDistributionPower;
	consts.ThinOccluderCompensation = settings.ThinOccluderCompensation;
	consts.FinalValuePower = settings.FinalValuePower;
	consts.DepthMIPSamplingOffset = settings.DepthMIPSamplingOffset;
	consts.NoiseIndex = (settings.DenoisePasses > 0) ? (frameCount % 64) : (0);
	consts.Padding0 = 0;

	memcpy(vr.XeGTAOconstants[currFrame].allocInfo.pMappedData, &consts, sizeof(XeGTAO::GTAOConstants));
}
OO_OPTIMIZE_ON

void XeGTAORenderPass::CreatePSO()
{
	
	CreatePipeline(); // Dependency on GBuffer Init()
}

bool XeGTAORenderPass::SetupDependencies(RenderGraph& builder)
{
	auto& vr = *VulkanRenderer::get();

	builder.Read(vr.attachments.gbuffer[GBufferAttachmentIndex::DEPTH]);
	builder.Read(vr.attachments.gbuffer[GBufferAttachmentIndex::NORMAL]);
	builder.Write(vr.attachments.SSAO_finalTarget, rhi::UAV);

	return true;
}

void XeGTAORenderPass::Draw(const VkCommandBuffer cmdlist)
{
	auto& vr = *VulkanRenderer::get();
	auto currFrame = vr.getFrame();
	auto* windowPtr = vr.windowPtr;

	lastCmd = cmdlist;
	PROFILE_GPU_CONTEXT(cmdlist);
	PROFILE_GPU_EVENT("XeGTAO");

	auto& attachments = vr.attachments.gbuffer;

	auto swapchainExt = vr.m_swapchain.swapChainExtent;
	glm::uvec2 m_size = { vr.renderWidth,vr.renderHeight};

	
	assert(outputAO->GetSize() == inputDepth->GetSize());
	assert(inputDepth->GetSampleCount() == 1); // MSAA no longer supported!

	// m_generateNormals |= inputNormals == nullptr;   // if normals not provided, we must generate them ourselves

	//m_outputBentNormals = outputBentNormals;

	// if (!m_outputBentNormals) m_debugShowBentNormals = false;
	// 
	// // when using bent normals, use R32_UINT, otherwise use R8_UINT; these could be anything else as long as the shading side matches
	// if (outputBentNormals)
	// {
	// 	assert(outputAO->GetUAVFormat() == vaResourceFormat::R32_UINT);
	// }
	// else
	// {
	// 	assert(outputAO->GetUAVFormat() == vaResourceFormat::R8_UINT);
	// }

	//UpdateTexturesAndShaders(inputDepth->GetSizeX(), inputDepth->GetSizeY());


	// if (inputNormals != nullptr)
	// {
	// 	assert(((inputNormals->GetSizeX() == m_size.x) || (inputNormals->GetSizeX() == m_size.x - 1)) && ((inputNormals->GetSizeY() == m_size.y) || (inputNormals->GetSizeY() == m_size.y - 1)));
	// }
	// assert(!m_shadersDirty); if (m_shadersDirty) return vaDrawResultFlags::UnspecifiedError;

	XeGTAO::GTAOConstants consts;
	XeGTAO::GTAOSettings settings;
	UpdateConstants(consts,settings);

	// constants used by all/some passes
	//computeItem.ConstantBuffers[0] = m_constantBuffer;
	// SRVs used by all/some passes
	//computeItem.ShaderResourceViews[5] = m_hilbertLUT;

	// needed only for shader debugging viz
	//vaDrawAttributes drawAttributes(camera);

	//if (m_generateNormals)
	//{
	//	VA_TRACE_CPUGPU_SCOPE(GenerateNormals, renderContext);
	//
	//	computeItem.ComputeShader = m_CSGenerateNormals;
	//
	//	// input SRVs
	//	computeItem.ShaderResourceViews[0] = inputDepth;
	//
	//	computeItem.SetDispatch((m_size.x + XE_GTAO_NUMTHREADS_X - 1) / XE_GTAO_NUMTHREADS_X, (m_size.y + XE_GTAO_NUMTHREADS_Y - 1) / XE_GTAO_NUMTHREADS_Y, 1);
	//
	//	renderContext.ExecuteSingleItem(computeItem, vaRenderOutputs::FromUAVs(m_workingNormals), &drawAttributes);
	//}

	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.pNext = NULL;
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // for shader
	viewCreateInfo.format = vr.attachments.xegtao_workingDepths.format;
	viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	viewCreateInfo.subresourceRange.levelCount = 1;
	viewCreateInfo.subresourceRange.layerCount = vr.attachments.xegtao_workingDepths.layerCount;
	viewCreateInfo.subresourceRange.baseMipLevel = 0;

	viewCreateInfo.image = vr.attachments.xegtao_workingDepths.image.image;

	std::array < VkImageView, XE_GTAO_DEPTH_MIP_LEVELS> mipViews;
	for (size_t i = 0; i < XE_GTAO_DEPTH_MIP_LEVELS; i++)
	{
		viewCreateInfo.subresourceRange.baseMipLevel = (uint32_t)i;
		vkCreateImageView(vr.m_device.logicalDevice, &viewCreateInfo, nullptr, &mipViews[i]);
	}
	DelayedDeleter::get()->DeleteAfterFrames([mipViews = mipViews,device = vr.m_device.logicalDevice]() 
	{
	for (size_t i = 0; i < XE_GTAO_DEPTH_MIP_LEVELS; i++)
		vkDestroyImageView(device, mipViews[i], nullptr);
	});

	std::array<VkDescriptorImageInfo, XE_GTAO_DEPTH_MIP_LEVELS> samplers{};
	for (size_t i = 0; i < samplers.size(); i++)
	{
		samplers[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		samplers[i].imageView = mipViews[0];
		samplers[i].sampler = GfxSamplerManager::GetSampler_EdgeClamp();
	}
	for (size_t i = 0; i < vr.attachments.xegtao_workingDepths.mipLevels; i++)
	{
		samplers[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		samplers[i].imageView = mipViews[i];
		samplers[i].sampler = GfxSamplerManager::GetSampler_EdgeClamp();
	}

	rhi::CommandList cmd{ cmdlist, "XeGTAO", oGFX::Colors::ORANGE };
	{
		cmd.BindPSO(pso_xegtao[(uint8_t)XEGTAO::GEN_NORMS]
			, PSOLayoutDB::xegtao_PSOLayouts[(uint8_t)XEGTAO::GEN_NORMS], VK_PIPELINE_BIND_POINT_COMPUTE);

		// input SRVs
		cmd.DescriptorSetBegin(0)
			.BindBuffer(0, vr.XeGTAOconstants[currFrame].getBufferInfoPtr(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
			.BindImage(1, &vr.attachments.gbuffer[GBufferAttachmentIndex::DEPTH], VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
			.BindSampler(2, GfxSamplerManager::GetSampler_PointClamp())
			.BindImage(3, &vr.attachments.xegtao_genNormals, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
			.BindImage(4, &normalA, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
			.BindImage(5, &normalB, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
			.BindImage(6, &vr.attachments.gbuffer[GBufferAttachmentIndex::NORMAL], VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
			.BindSampler(7, GfxSamplerManager::GetSampler_LinearClamp())
			//.BindImage(3, &vr.attachments.xegtao_workingDepths, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
			;

		cmd.Dispatch((m_size.x + XE_GTAO_NUMTHREADS_X - 1) / XE_GTAO_NUMTHREADS_X, (m_size.y + XE_GTAO_NUMTHREADS_Y - 1) / XE_GTAO_NUMTHREADS_Y, 1);
	}

	{
		cmd.BindPSO(pso_xegtao[(uint8_t)XEGTAO::PREFILTER_DEPTHS]
			, PSOLayoutDB::xegtao_PSOLayouts[(uint8_t)XEGTAO::PREFILTER_DEPTHS], VK_PIPELINE_BIND_POINT_COMPUTE);

		// input SRVs
		cmd.DescriptorSetBegin(0)
			.BindBuffer(0, vr.XeGTAOconstants[currFrame].getBufferInfoPtr(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
			.BindImage(1, &vr.attachments.gbuffer[GBufferAttachmentIndex::DEPTH], VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
			.BindSampler(2, GfxSamplerManager::GetSampler_PointClamp())
			.BindImage(3, samplers.data()+0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
			.BindImage(4, samplers.data()+1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
			.BindImage(5, samplers.data()+2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
			.BindImage(6, samplers.data()+3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
			.BindImage(7, samplers.data()+4, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
			//.BindImage(3, &vr.attachments.xegtao_workingDepths, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
			;

		// note: in CSPrefilterDepths16x16 each is thread group handles a 16x16 block (with [numthreads(8, 8, 1)] and each logical thread handling a 2x2 block)
		cmd.Dispatch((m_size.x + 16 - 1) / 16, (m_size.y + 16 - 1) / 16, 1);
	}

	{
		cmd.BindPSO(pso_xegtao[(uint8_t)XEGTAO::MAIN_PASS]
			, PSOLayoutDB::xegtao_PSOLayouts[(uint8_t)XEGTAO::MAIN_PASS], VK_PIPELINE_BIND_POINT_COMPUTE);

		// input SRVs
		cmd.DescriptorSetBegin(0)
			.BindBuffer(0, vr.XeGTAOconstants[currFrame].getBufferInfoPtr(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)

			.BindImage(1, &vr.attachments.xegtao_workingDepths, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)

			.BindSampler(2, GfxSamplerManager::GetSampler_PointClamp())

			.BindImage(3, &vr.attachments.xegtao_hilbert, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
			.BindImage(4, &vr.attachments.gbuffer[GBufferAttachmentIndex::NORMAL], VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
			.BindImage(5, &vr.attachments.xegtao_workingAOTerm, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
			.BindImage(6, &vr.attachments.xegtao_workingEdges, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
			;

		cmd.Dispatch((m_size.x + XE_GTAO_NUMTHREADS_X - 1) / XE_GTAO_NUMTHREADS_X, (m_size.y + XE_GTAO_NUMTHREADS_Y - 1) / XE_GTAO_NUMTHREADS_Y, 1);
	}

	{
		vkutils::Texture* aoTerm = &vr.attachments.xegtao_workingAOTerm;
		vkutils::Texture* aoTermPong = &vr.attachments.xegtao_workingAOTermPong;

		const int passCount = std::max(1, settings.DenoisePasses); // even without denoising we have to run a single last pass to output correct term into the external output texture
		for (int i = 0; i < passCount; i++)
		{
			const bool lastPass = i == passCount - 1;

			if (lastPass) {
				cmd.BindPSO(pso_xegtao[(uint8_t)XEGTAO::DENOISE_LAST]
					, PSOLayoutDB::xegtao_PSOLayouts[(uint8_t)XEGTAO::DENOISE_LAST], VK_PIPELINE_BIND_POINT_COMPUTE);
			}
			else {
				cmd.BindPSO(pso_xegtao[(uint8_t)XEGTAO::DENOISE]
					, PSOLayoutDB::xegtao_PSOLayouts[(uint8_t)XEGTAO::DENOISE], VK_PIPELINE_BIND_POINT_COMPUTE);
			}
			cmd.DescriptorSetBegin(0)
				.BindBuffer(0, vr.XeGTAOconstants[currFrame].getBufferInfoPtr(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)

				.BindImage(1, aoTerm, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
				.BindImage(2, &vr.attachments.xegtao_workingEdges, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)

				.BindSampler(3, GfxSamplerManager::GetSampler_PointClamp())

				.BindImage(4, aoTermPong, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
				;

			cmd.Dispatch((m_size.x + XE_GTAO_NUMTHREADS_X - 1) / XE_GTAO_NUMTHREADS_X, (m_size.y + XE_GTAO_NUMTHREADS_Y - 1) / XE_GTAO_NUMTHREADS_Y, 1);
			std::swap(aoTerm, aoTermPong);
		}

		cmd.CopyImage(aoTerm, &vr.attachments.SSAO_finalTarget);
	}


	//{
	//	VA_TRACE_CPUGPU_SCOPE(MainPass, renderContext);
	//
	//	shared_ptr<vaComputeShader> shaders[] = { m_CSGTAOLow, m_CSGTAOMedium, m_CSGTAOHigh, m_CSGTAOUltra };
	//	computeItem.ComputeShader = shaders[m_settings.QualityLevel];
	//
	//	// input SRVs
	//	computeItem.ShaderResourceViews[0] = m_workingDepths;
	//	computeItem.ShaderResourceViews[1] = (m_generateNormals) ? (m_workingNormals) : (inputNormals);
	//
	//	computeItem.SetDispatch((m_size.x + XE_GTAO_NUMTHREADS_X - 1) / XE_GTAO_NUMTHREADS_X, (m_size.y + XE_GTAO_NUMTHREADS_Y - 1) / XE_GTAO_NUMTHREADS_Y, 1);
	//
	//	renderContext.ExecuteSingleItem(computeItem, vaRenderOutputs::FromUAVs(m_workingAOTerm, m_workingEdges, m_debugImage), &drawAttributes);
	//}

	//{
	//	VA_TRACE_CPUGPU_SCOPE(Denoise, renderContext);
	//	const int passCount = std::max(1, m_settings.DenoisePasses); // even without denoising we have to run a single last pass to output correct term into the external output texture
	//	for (int i = 0; i < passCount; i++)
	//	{
	//		const bool lastPass = i == passCount - 1;
	//		VA_TRACE_CPUGPU_SCOPE(DenoisePass, renderContext);
	//
	//		computeItem.ComputeShader = (lastPass) ? (m_CSDenoiseLastPass) : (m_CSDenoisePass);
	//
	//		// input SRVs
	//		computeItem.ShaderResourceViews[0] = m_workingAOTerm;   // see std::swap below
	//		computeItem.ShaderResourceViews[1] = m_workingEdges;
	//
	//		computeItem.SetDispatch((m_size.x + (XE_GTAO_NUMTHREADS_X * 2) - 1) / (XE_GTAO_NUMTHREADS_X * 2), (m_size.y + XE_GTAO_NUMTHREADS_Y - 1) / XE_GTAO_NUMTHREADS_Y, 1);
	//
	//		renderContext.ExecuteSingleItem(computeItem, vaRenderOutputs::FromUAVs((lastPass) ? (outputAO) : (m_workingAOTermPong), nullptr, m_debugImage), &drawAttributes);
	//		std::swap(m_workingAOTerm, m_workingAOTermPong);      // ping becomes pong, pong becomes ping.
	//	}
	//}

}

void XeGTAORenderPass::Shutdown()
{

	auto& vr = *VulkanRenderer::get();
	auto& device = vr.m_device.logicalDevice;
	const size_t MAX_FRAMES = 2;

	for (size_t i = 0; i < (uint8_t)XEGTAO::MAX_SIZE; i++)
	{
		vkDestroyPipelineLayout(device, PSOLayoutDB::xegtao_PSOLayouts[i], nullptr);
		vkDestroyPipeline(device, pso_xegtao[i], nullptr);
	}

	vr.attachments.xegtao_hilbert.destroy();
	vr.attachments.xegtao_workingDepths.destroy();
	vr.attachments.xegtao_workingEdges.destroy();
	vr.attachments.xegtao_workingAOTerm.destroy();
	vr.attachments.xegtao_workingAOTermPong.destroy();
	vr.attachments.xegtao_genNormals.destroy();

	normalA.destroy();
	normalB.destroy();

	for (size_t i = 0; i < MAX_FRAMES; i++)
	{
		vmaDestroyBuffer(vr.m_device.m_allocator, vr.XeGTAOconstants[i].buffer, vr.XeGTAOconstants[i].alloc);
	}

}

void XeGTAORenderPass::InitRandomFactors()
{
	auto& vr = *VulkanRenderer::get();
	auto swapchainext = vr.m_swapchain.swapChainExtent;

	{
		uint16_t data[64 * 64];
		for (int x = 0; x < 64; x++)
			for (int y = 0; y < 64; y++)
			{
				uint32_t r2index = XeGTAO::HilbertIndex(x, y);
				assert(r2index < 65536);
				data[x + 64 * y] = (uint16_t)r2index;
			}

		VkBufferImageCopy copyRegion{};
		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.bufferOffset = 0;
		copyRegion.imageExtent.width = 64;
		copyRegion.imageExtent.height = 64;
		copyRegion.imageExtent.depth = 1;
		std::vector<VkBufferImageCopy> copies{ copyRegion };

		vr.attachments.xegtao_hilbert.name = "HILBERT_LUT";
		vr.attachments.xegtao_hilbert.fromBuffer(data, 64 * 64 * sizeof(uint16_t), VK_FORMAT_R16_UINT,
			64, 64, copies, &vr.m_device, vr.m_device.graphicsQueue, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_FILTER_NEAREST);
	}
}


void XeGTAORenderPass::CreateDescriptors()
{
	//if (m_log)
	//{
	//	std::cout << __FUNCSIG__ << std::endl;
	//}

	auto& vr = *VulkanRenderer::get();
	// At this point, all dependent resources (gbuffer etc) must be ready.
	auto& attachments = vr.attachments.gbuffer;
	
	DescriptorBuilder::Begin()
		.BindBuffer(0, nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)

		.BindImage(1, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)

		.BindImage(2, nullptr, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // point clamp

		.BindImage(3, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(4, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(5, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(6, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(7, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)

		.BuildLayout(SetLayoutDB::compute_xegtao[(uint8_t)XEGTAO::PREFILTER_DEPTHS]);

	DescriptorBuilder::Begin()
		.BindBuffer(0, nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)

		.BindImage(1, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(2, nullptr, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)

		.BindImage(3, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT) // point clamp

		.BindImage(4, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(5, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(6, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)

		.BuildLayout(SetLayoutDB::compute_xegtao[(uint8_t)XEGTAO::MAIN_PASS]);

	// denoise
	DescriptorBuilder::Begin()
		.BindBuffer(0, nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)

		.BindImage(1, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(2, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)

		.BindImage(3, nullptr, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // point clamp

		.BindImage(4, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)

		.BuildLayout(SetLayoutDB::compute_xegtao[(uint8_t)XEGTAO::DENOISE]);

	DescriptorBuilder::Begin()
		.BindBuffer(0, nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)

		.BindImage(1, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.BindImage(2, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)

		.BindImage(3, nullptr, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT) // point clamp

		.BindImage(4, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)

		.BuildLayout(SetLayoutDB::compute_xegtao[(uint8_t)XEGTAO::DENOISE_LAST]);


	//gen norms
	DescriptorBuilder::Begin()
		.BindBuffer(0, nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)

		.BindImage(1, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)

		.BindImage(2, nullptr, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)

		.BindImage(3, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT) // point clamp
		.BindImage(4, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT) // point clamp
		.BindImage(5, nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT) // point clamp
		.BindImage(6, nullptr, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT) // point clamp
		.BindImage(7, nullptr, VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT)

		.BuildLayout(SetLayoutDB::compute_xegtao[(uint8_t)XEGTAO::GEN_NORMS]);

}

void XeGTAORenderPass::CreatePipelineLayout()
{
	auto& vr = *VulkanRenderer::get();
	auto& m_device = vr.m_device;

	// setup all layouts
	for (size_t i = 0; i < (uint8_t)XEGTAO::MAX_SIZE; i++)
	{
		std::vector<VkDescriptorSetLayout> setLayouts
		{
			SetLayoutDB::compute_xegtao[i],
		};

		VkPipelineLayoutCreateInfo plci = oGFX::vkutils::inits::pipelineLayoutCreateInfo(
			setLayouts.data(), static_cast<uint32_t>(setLayouts.size()));

		VkPushConstantRange pushConstantRange{ VK_SHADER_STAGE_ALL, 0, 128 };
		plci.pushConstantRangeCount = 1;
		plci.pPushConstantRanges = &pushConstantRange;
		std::string name(xegtao_shaders_names[i]);
		name += "_PSOLayout";
		VK_CHK(vkCreatePipelineLayout(m_device.logicalDevice, &plci, nullptr, &PSOLayoutDB::xegtao_PSOLayouts[i]));
		VK_NAME(m_device.logicalDevice, name.c_str(), PSOLayoutDB::xegtao_PSOLayouts[i]);
	}
}

void XeGTAORenderPass::SetupRenderpass()
{
	auto& vr = *VulkanRenderer::get();

	CreateDescriptors();
	CreatePipelineLayout();
}

void XeGTAORenderPass::CreatePipeline()
{
	auto& vr = *VulkanRenderer::get();
	auto& m_device = vr.m_device;

	VkComputePipelineCreateInfo computeCI;

	for (size_t i = 0; i < (uint8_t)XEGTAO::MAX_SIZE; i++)
	{
		VkPipeline& pipe = pso_xegtao[i];
		if (pipe != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(m_device.logicalDevice, pipe, nullptr);
		}
		const char* shader = xegtao_shaders[i];
		computeCI = oGFX::vkutils::inits::computeCreateInfo(PSOLayoutDB::xegtao_PSOLayouts[i]);
		computeCI.stage = vr.LoadShader(m_device, shader, VK_SHADER_STAGE_COMPUTE_BIT);
		VK_CHK(vkCreateComputePipelines(m_device.logicalDevice, VK_NULL_HANDLE, 1, &computeCI, nullptr, &pipe));
		std::string name(xegtao_shaders_names[i]);
		name += "_PSO";
		VK_NAME(m_device.logicalDevice, name.c_str(), &pipe);
		vkDestroyShaderModule(m_device.logicalDevice, computeCI.stage.module, nullptr); // destroy shader
	}

}
