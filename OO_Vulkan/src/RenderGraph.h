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
#include "VulkanTexture.h"
#include "RGResource.h"
#include "rhi/CommandList.h"

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

		void Write(vkutils::Texture*, rhi::ResourceUsage);
		void Write(vkutils::Texture&, rhi::ResourceUsage);
		void Read(vkutils::Texture*, rhi::ResourceUsage = rhi::ResourceUsage::SRV);
		void Read(vkutils::Texture&, rhi::ResourceUsage = rhi::ResourceUsage::SRV);

		RGTextureRef RegisterExternalTexture(vkutils::Texture& texture);
	private:
		using TextureRegistry = std::unordered_map<vkutils::Texture*, rhi::ImageStateTracking>;

		struct PassInfo {
			std::string name;
			GfxRenderpass* pass;

			TextureRegistry textureReg;
		};

		std::vector<PassInfo> passes;
		std::vector<std::shared_ptr<RGTexture>> textures;

		uint32_t currentPass = 0;

		std::vector<TextureRegistry>m_trackedTextures;

		//std::unordered_map<VkBuffer, rhi::BufferStateTracking> m_trackedBuffers;
	};

