#include "GraphicsBatch.h"

#include "VulkanRenderer.h"
#include "GraphicsWorld.h"
#include "gpuCommon.h"
#include <cassert>

GraphicsBatch GraphicsBatch::Init(GraphicsWorld* gw, VulkanRenderer* renderer, size_t maxObjects)
{
	GraphicsBatch gb;
	gb.m_world = gw;
	gb.m_renderer = renderer;

	for (auto& batch : gb.m_batches)
	{
		batch.reserve(maxObjects);
	}

	return gb;
}

void GraphicsBatch::GenerateBatches()
{
	auto& entities = m_world->GetAllObjectInstances();

	for (auto& batch : m_batches)
	{
		batch.clear();
	}

	for (auto& ent: entities)
	{
		auto& model = m_renderer->models[ent.modelID];
		using Batch = GraphicsBatch::DrawBatch;
		using Flags = ObjectInstanceFlags;
		if (ent.flags & Flags::SHADOW_CASTER)
		for (auto& node :model.nodes)
		{
			// TODO: yes
			//oGFX::IndirectCommandsHelper(node, m_batches[Batch::SHADOW_CAST], indirectDebugCommandsCPU,m);			
		}
	}

}

const std::vector<oGFX::IndirectCommand>& GraphicsBatch::GetBatch(int32_t batchIdx)
{
	assert(batchIdx < -1 && batchIdx < GraphicsBatch::MAX_NUM);

	return m_batches[batchIdx];
}
