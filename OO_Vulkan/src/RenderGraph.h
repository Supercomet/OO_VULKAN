/************************************************************************************//*!
\file           RenderGraph.h
\project        Ouroboros
\author         Jamie Kong, j.kong, 390004720 | code contribution (100%)
\par            email: j.kong\@digipen.edu
\date           Apr 24, 2024
\brief              A texture wrapper object for vulkan texture

Copyright (C) 2022 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*//*************************************************************************************/
#pragma once

#include <string>
#include <vector>
#include "VulkanDevice.h"

class GfxRenderpass;

	class RenderGraph
	{
	public:
		std::string name{}; // maybe remove when not debug?
		VulkanDevice* device{ nullptr };
		RenderGraph();

		void AddPass(GfxRenderpass* pass);

		void Setup();
		void Compile();
		void Execute();
	private:
		std::vector<GfxRenderpass*> passes;		
	};

