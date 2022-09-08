#pragma once

#include "GfxRenderpass.h"
#include "vulkan/vulkan.h"

#include <array>

class DebugDrawRenderpass : public GfxRenderpass
{
public:

	VkRenderPass debugRenderpass{};

	void Init() override;
	void Draw() override;
	void Shutdown() override;

private:
	void CreateDebugRenderpass();
	void CreatePipeline();
	void InitDebugBuffers();

	VkPipeline debugDrawLinesPSO{};
	VkPipeline debugDrawLinesPSO_NoDepthTest{};

	struct DebugDrawPSOSelector
	{
		std::array<VkPipeline, 6> psos;

		// Ghetto... Need a more robust solution
		VkPipeline GetPSO(bool isDepthTest, bool isWireframe, bool isPoint)
		{
			int i = isWireframe ? 1 : 0;
			i = isPoint ? 2 : i;
			int j = isDepthTest ? 1 : 0;
			return psos[i + 2 * j];
		}

	}m_DebugDrawPSOSelector;
};

