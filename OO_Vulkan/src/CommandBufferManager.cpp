/************************************************************************************//*!
\file           CommandBufferManager.cpp
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
#include "gpuCommon.h"
#include "VulkanDevice.h"
#include "VulkanUtils.h"
#include "CommandBufferManager.h"

VkResult oGFX::CommandBufferManager::InitPool(VkDevice device, uint32_t queueIndex)
{
    assert(device);
    m_device = device;

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueIndex;

    VkResult res{};
    res = vkCreateCommandPool(device, &poolInfo, nullptr, &m_commandpool);
    VK_CHK(res);
    VK_NAME(device, "commandPool", m_commandpool);

    return res;
}

VkCommandBuffer oGFX::CommandBufferManager::GetCommandBuffer(bool begin)
{
    if (nextIdx == commandBuffers.size()) 
    {
        AllocateCommandBuffer();
    }
    VkCommandBuffer result = commandBuffers[nextIdx++];
    if (begin) {
        VkCommandBufferBeginInfo cmdBufInfo = oGFX::vkutils::inits::commandBufferBeginInfo();
        vkBeginCommandBuffer(result, &cmdBufInfo);
    }
    return result;
}

void oGFX::CommandBufferManager::ResetPool()
{
    nextIdx = 0;

    VkCommandPoolResetFlags flags{ VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT };

    VK_CHK(vkResetCommandPool(m_device, m_commandpool, flags));
}

VkCommandPool oGFX::CommandBufferManager::GetCommandPool()
{
    return m_commandpool;
}

void oGFX::CommandBufferManager::DestroyPool()
{
    if (m_commandpool);
    vkDestroyCommandPool(m_device, m_commandpool, nullptr);
    m_commandpool = VK_NULL_HANDLE;
}

void oGFX::CommandBufferManager::SubmitCommandBuffer(VkQueue queue, VkCommandBuffer cmd)
{
    // End commands
    // TODO: Perhaps dont end here?
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    //assuming we have only a few meshes to load we will pause here until we load the previous object
    //submit transfer commands to transfer queue and wait until it finishes
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
}

VkCommandBuffer oGFX::CommandBufferManager::TEMP_GET_FIRST_CMD()
{
    if (commandBuffers.empty())
    {
        AllocateCommandBuffer();
    }
    return commandBuffers.front();
}

void oGFX::CommandBufferManager::AllocateCommandBuffer()
{
	VkCommandBufferAllocateInfo cbAllocInfo = {};
	cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// VK_COMMAND_BUFFER_LEVEL_PRIMARY : buffer you submit directly to queue, cant be called  by other buffers
	//VK_COMMAND_BUFFER_LEVEL_SECONDARY :  buffer cant be called directly, can be called from other buffers via "vkCmdExecuteCommands" when recording commands in primary buffer
	cbAllocInfo.commandBufferCount = 1;
	cbAllocInfo.commandPool = m_commandpool;

	VkCommandBuffer cb;

	VkResult result = vkAllocateCommandBuffers(m_device, &cbAllocInfo, &cb);
	if (result != VK_SUCCESS)
	{
		std::cerr << "Failed to allocate Command Buffers!" << std::endl;
		__debugbreak();
	}

    commandBuffers.emplace_back(cb);
}

