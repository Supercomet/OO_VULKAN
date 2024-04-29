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
#include "VulkanTexture.h"
#include "VulkanDevice.h"

class RGResource {
	void* resource = nullptr;
	void* GetResource();
};

class RGTextureDesc {
public:
	uint32_t width;
	uint32_t height;
	VkFormat format;
	uint8_t numMips = 1;
	uint16_t numArray = 1;	
};

class RGTexture : RGResource
{
	public:
		vkutils::Texture* textureRef;
};

using RGTextureRef = RGTexture*;


enum ResourceUsage {
	UNKNOWN,
	SRV,
	UAV,
	ATTACHMENT
};

struct ImageStateTracking {
	VkImageLayout referenceLayout{ VK_IMAGE_LAYOUT_UNDEFINED };
	VkImageLayout currentLayout{ VK_IMAGE_LAYOUT_UNDEFINED };
	VkImageLayout expectedLayout{ VK_IMAGE_LAYOUT_UNDEFINED };
};

struct BufferStateTracking {
	ResourceUsage referenceAccess{ SRV };
	ResourceUsage currentAccess{ SRV };
	ResourceUsage expectedAccess{ UNKNOWN };
	VkPipelineBindPoint previousStage{ VK_PIPELINE_BIND_POINT_GRAPHICS };
};


