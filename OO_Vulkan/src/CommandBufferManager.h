/************************************************************************************//*!
\file           CommandBufferManager.h
\project        Ouroboros
\author         Jamie Kong, j.kong, 390004720 | code contribution (100%)
\par            email: j.kong\@digipen.edu
\date           Aug 24, 2023
\brief              Defines command list allocator

Copyright (C) 2022 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*//*************************************************************************************/
#pragma once
#include "vulkan/vulkan.h"
#include <vector>

namespace oGFX
{

class CommandBufferManager
{
public:
	VkResult InitPool(VkDevice device, uint32_t queueIndex);
	VkCommandBuffer GetCommandBuffer(bool begin = false);
	void ResetPool();
	VkCommandPool GetCommandPool();
	void DestroyPool();
	void SubmitCommandBuffer(VkQueue queue, VkCommandBuffer cmd);

	VkCommandBuffer TEMP_GET_FIRST_CMD();

private:
	void AllocateCommandBuffer();

	VkDevice m_device{};
	uint32_t nextIdx{};
	VkCommandPool m_commandpool{};
	std::vector<VkCommandBuffer>commandBuffers;
};


}
