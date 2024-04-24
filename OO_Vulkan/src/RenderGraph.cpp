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

RenderGraph::RenderGraph()
{
}

void RenderGraph::AddPass(GfxRenderpass* pass)
{
	passes.push_back(pass);
}

void RenderGraph::Setup()
{
}

void RenderGraph::Compile()
{
}

void RenderGraph::Execute()
{
	auto& vr = *VulkanRenderer::get();
	for (auto& pass : passes)
	{
		auto renderTask = [vr = &vr, pass = pass](void*) {
			//generally works until we need to perform better frustrum culling....
			const VkCommandBuffer cmd = vr->GetCommandBuffer();
			pass->Draw(cmd);
			};
		vr.m_taskList.push(Task(renderTask, nullptr, &vr.drawCallRecrodingCompleted));

		auto sequencer = [vr = &vr, pass = pass](void*) {
			OO_ASSERT(pass->lastCmd);
			vr->QueueCommandBuffer(pass->lastCmd);
			};
		vr.m_sequentialTasks.emplace_back(sequencer);
	}
}
