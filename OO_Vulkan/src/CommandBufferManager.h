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

	enum eRECSTATUS : int8_t {
		INVALID = -1,
		RECORDING = 0,
		ENDED = 1,
		SUBMITTED = 2,
};

class CommandBufferManager
{
public:
	VkResult InitPool(VkDevice device, uint32_t queueIndex);
	VkCommandBuffer GetNextCommandBuffer(uint32_t order, uint32_t threadID = 0,bool begin = false);
	void EndCommandBuffer(uint32_t thread_id, VkCommandBuffer cmd);
	void ResetPools();
	void DestroyPools();
	void SubmitCommandBuffer(uint32_t thread_id, VkQueue queue, VkCommandBuffer cmd);
	void SubmitCommandBufferAndWait(uint32_t thread_id, VkQueue queue, VkCommandBuffer cmd);
	void SubmitAll(VkQueue queue, VkSubmitInfo submitInfo = {}, VkFence signalFence = VK_NULL_HANDLE);

	std::vector <VkCommandPool> m_commandpools{};
private:
	size_t FindCmdIdx(uint32_t thread_id, VkCommandBuffer cmd);
	void AllocateCommandBuffer(uint32_t thread_id);

	VkDevice m_device{};
	std::vector<uint32_t> nextIndices{};
	std::vector<std::vector<VkCommandBuffer>> threadCBs;
	std::vector<std::vector<eRECSTATUS>> threadSubmitteds{};
	std::vector<std::vector<uint32_t>> threadOrder{};
};


}
