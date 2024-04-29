/************************************************************************************//*!
\file           RenderGraph.cpp
\project        Ouroboros
\author         Jamie Kong, j.kong, 390004720 | code contribution (100%)
\par            email: j.kong\@digipen.edu
\date           Apr 04, 2024
\brief              Defines the texture wrapper object

Copyright (C) 2022 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*//*************************************************************************************/
#include "RenderGraph.h"
#include "VulkanUtils.h"

#include "GfxRenderpass.h"
#include "VulkanRenderer.h"

OO_OPTIMIZE_OFF
RenderGraph::RenderGraph()
{
}

void RenderGraph::AddPass(GfxRenderpass* pass)
{
	PassInfo passInfo{};
	passInfo.pass = pass;
	passInfo.name = pass->name;
	passes.push_back(passInfo);
}

void RenderGraph::Setup()
{
	m_trackedTextures.resize(passes.size());
	currentPass = 0;
	for (PassInfo& passInfo : passes)
	{
		GfxRenderpass* pass = passInfo.pass;
		pass->SetupDependencies(*this);
		currentPass++;
	}
}

void RenderGraph::Compile()
{
}

void RenderGraph::Execute()
{
	auto& vr = *VulkanRenderer::get();
	for (PassInfo& passInfo : passes)
	{
		GfxRenderpass* pass = passInfo.pass;
		auto renderTask = [vr = &vr, pass = pass](void*) {
			const VkCommandBuffer cmd = vr->GetCommandBuffer();
			pass->Draw(cmd);
			};
		vr.m_taskList.push(Task(renderTask, nullptr, &vr.drawCallRecordingCompleted));

		auto sequencer = [vr = &vr, pass = pass](void*) {
			OO_ASSERT(pass->lastCmd);
			vr->QueueCommandBuffer(pass->lastCmd);
			};
		vr.m_sequentialTasks.emplace_back(sequencer);
	}
}

void RenderGraph::Write(vkutils::Texture* t, ResourceUsage usage)
{
	OO_ASSERT(t);
	OO_ASSERT(usage == UAV || usage == ATTACHMENT); // should there be anything else?
	//ensure tracking image globally ??
	VkImageLayout imgFormat = VK_IMAGE_LAYOUT_UNDEFINED;
	if (usage == ATTACHMENT) 
	{
		imgFormat = t->format == VulkanRenderer::G_DEPTH_FORMAT ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
	else {
		imgFormat = VK_IMAGE_LAYOUT_GENERAL;
	}
	passes[currentPass].textureReg[t].expectedLayout = imgFormat;
}

void RenderGraph::Write(vkutils::Texture& t, ResourceUsage u)
{
	Write(&t, u);
}

void RenderGraph::Read(vkutils::Texture* t, ResourceUsage usage)
{
	OO_ASSERT(t);
	OO_ASSERT(usage == SRV); // should there be anything else?
	//ensure tracking image globally ??
	passes[currentPass].textureReg[t].expectedLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void RenderGraph::Read(vkutils::Texture& t, ResourceUsage u)
{
	Read(&t, u);
}

void RenderGraph::Read(oGFX::AllocatedBuffer* buffer)
{
	OO_ASSERT(buffer);
	OO_ASSERT(buffer->buffer); // invalud buffer

	//ensure tracking buffer globally ??
	passes[currentPass].bufferReg[buffer].expectedAccess = ResourceUsage::SRV;
}

void RenderGraph::Read(oGFX::AllocatedBuffer& buffer)
{
	Read(&buffer);
}

void RenderGraph::Write(oGFX::AllocatedBuffer* buffer, ResourceUsage usage)
{
	OO_ASSERT(buffer);
	OO_ASSERT(buffer->buffer); // invalud buffer
	OO_ASSERT(usage == ResourceUsage::UAV); // should there be another??

	//ensure tracking buffer globally ??
	passes[currentPass].bufferReg[buffer].expectedAccess = ResourceUsage::UAV;
}

void RenderGraph::Write(oGFX::AllocatedBuffer& buffer, ResourceUsage)
{
}

void RenderGraph::DumpPassDependencies()
{
	printf("=============\n");
	for (PassInfo& passInfo : passes)
	{
		GfxRenderpass* pass = passInfo.pass;
		printf("Setting up pass %s\n", pass->name.c_str());
		printf("\tPass Requested:\n");
		for (auto& kvp : passInfo.textureReg)
		{
			const char* state = nullptr;
			switch (kvp.second.expectedLayout)
			{
			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				state = "SRV";
				break;
			case VK_IMAGE_LAYOUT_GENERAL:
				state = "UAV";
				break;
			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				state = "RT";
				break;
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				state = "RT Depth";
				break;
			default:
				state = "**UNDEF**";
				break;
			}
			printf("\t  Tex[%s] requested : %s\n", kvp.first->name.c_str(), state);
		}
		for (auto& kvp : passInfo.bufferReg)
		{
			const char* state = nullptr;
			
			switch (kvp.second.expectedAccess)
			{
			case ResourceUsage::SRV:
				state = "SRV";
				break;
			case ResourceUsage::UAV:
				state = "UAV";
				break;
			case ResourceUsage::ATTACHMENT:
				OO_ASSERT(state); // buffer shouldnt be attachment
				state = "RT";
				break;
			default:
				state = "**UNDEF**";
				break;
			}
			printf("\t  Buf[%s] requested : %s\n", kvp.first->name.c_str(), state);
		}
	}
	printf("=============\n");
	printf("\n");
}

RGTextureRef RenderGraph::RegisterExternalTexture(vkutils::Texture& texture)
{
	auto ptr = std::make_shared<RGTexture>();
	textures.push_back(ptr);
	return ptr.get();
}
