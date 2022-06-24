#include "GfxRenderpass.h"

RenderPassDatabase* RenderPassDatabase::ms_renderpass{ nullptr };

RenderPassDatabase::~RenderPassDatabase()
{
	if (ms_renderpass)
		delete ms_renderpass;
}

RenderPassDatabase* RenderPassDatabase::Get()
{
	if (ms_renderpass == nullptr)
	{
		ms_renderpass = new RenderPassDatabase;
	}
	return ms_renderpass;
}

void RenderPassDatabase::RegisterRenderPass(std::unique_ptr<GfxRenderpass>&& renderPass)
{
	renderPass->m_Index = m_RegisteredRenderPasses++;
	m_AllRenderPasses.emplace_back(std::move(renderPass));
}

void RenderPassDatabase::InitAllRegisteredPasses()
{
	auto renderpasses = Get();
	
	for (auto& renderPass : renderpasses->m_AllRenderPasses)
	{
		renderPass->Init();
	}
}

void RenderPassDatabase::ShutdownAllRegisteredPasses()
{
    auto renderpasses = Get();

    for (auto& renderPass : renderpasses->m_AllRenderPasses)
    {
        renderPass->Shutdown();
    }
}