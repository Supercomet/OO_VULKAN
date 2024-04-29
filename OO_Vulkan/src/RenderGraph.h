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
#include "GpuVector.h"
#include "VulkanDevice.h"
#include "VulkanTexture.h"
#include "RGResource.h"
#include "rhi/CommandList.h"

OO_OPTIMIZE_OFF
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

		void Write(vkutils::Texture*, ResourceUsage);
		void Write(vkutils::Texture&, ResourceUsage);
		void Read(vkutils::Texture*, ResourceUsage = SRV);
		void Read(vkutils::Texture&, ResourceUsage = SRV);

		template<typename T> void Write(GpuVector<T>* buffer, ResourceUsage= UAV);
		template<typename T> void Write(GpuVector<T>& buffer, ResourceUsage= UAV);
		template<typename T> void Read(GpuVector<T>* buffer);
		template<typename T> void Read(GpuVector<T>& buffer);

		void Write(oGFX::AllocatedBuffer* buffer, ResourceUsage= UAV);
		void Write(oGFX::AllocatedBuffer& buffer, ResourceUsage= UAV);
		void Read(oGFX::AllocatedBuffer* buffer);
		void Read(oGFX::AllocatedBuffer& buffer);

		void DumpPassDependencies();

		RGTextureRef RegisterExternalTexture(vkutils::Texture& texture);
	private:
		using TextureRegistry = std::unordered_map<vkutils::Texture*, ImageStateTracking>;
		using BufferRegistry = std::unordered_map<oGFX::AllocatedBuffer*, BufferStateTracking>;

		struct PassInfo {
			std::string name;
			GfxRenderpass* pass;

			TextureRegistry textureReg;
			BufferRegistry bufferReg;
		};

		std::vector<PassInfo> passes;
		std::vector<std::shared_ptr<RGTexture>> textures;

		uint32_t currentPass = 0;

		std::vector<TextureRegistry>m_trackedTextures;

		//std::unordered_map<VkBuffer, BufferStateTracking> m_trackedBuffers;
	};

	template<typename T>
	inline void RenderGraph::Write(GpuVector<T>* gpuvector, ResourceUsage usage)
	{
		Write(gpuvector->m_buffer, usage);
	}

	template<typename T>
	inline void RenderGraph::Write(GpuVector<T>& gpuvector, ResourceUsage usage)
	{
		Write(&gpuvector, usage);
	}

	template<typename T>
	inline void RenderGraph::Read(GpuVector<T>* gpuvector)
	{
		Read(gpuvector->m_buffer);
	}

	template<typename T>
	inline void RenderGraph::Read(GpuVector<T>& gpuvector)
	{
		Read(&gpuvector);
	}
