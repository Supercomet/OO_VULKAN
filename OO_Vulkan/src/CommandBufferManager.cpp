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
#include <algorithm>
#include <thread>
#include "VulkanDevice.h"
#include "VulkanUtils.h"
#include "CommandBufferManager.h"
#include "Profiling.h"

const size_t MAX_THREADS = std::thread::hardware_concurrency();

VkResult oGFX::CommandBufferManager::InitPool(VkDevice device, uint32_t queueIndex)
{
    assert(device);
    m_device = device;

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueIndex;

    nextIndices.resize(MAX_THREADS);
    threadSubmitteds.resize(MAX_THREADS);
    threadCBs.resize(MAX_THREADS);
    threadOrder.resize(MAX_THREADS);
    m_commandpools.resize(MAX_THREADS);
    for (size_t i = 0; i < MAX_THREADS; i++)
    {
        VK_CHK(vkCreateCommandPool(device, &poolInfo, nullptr, &m_commandpools[i]));
        VK_NAME(device, ("commandPool_t" + std::to_string(i)).c_str(), m_commandpools[i]);
    }  

    return VK_SUCCESS;
}

VkCommandBuffer oGFX::CommandBufferManager::GetNextCommandBuffer(uint32_t order, uint32_t thread_id, bool begin)
{
    auto& submitted = threadSubmitteds[thread_id];
    auto& nextIndex = nextIndices[thread_id];
    auto& commandBuffers = threadCBs[thread_id];
    auto& buffOrder = threadOrder[thread_id];

    if (nextIndex == commandBuffers.size())
    {
        AllocateCommandBuffer(thread_id);
    }
    auto bufferIdx = nextIndex++;
    VkCommandBuffer result = commandBuffers[bufferIdx];
    if (begin) {
        VkCommandBufferBeginInfo cmdBufInfo = oGFX::vkutils::inits::commandBufferBeginInfo();
        vkBeginCommandBuffer(result, &cmdBufInfo);
        submitted[bufferIdx] = eRECSTATUS::RECORDING;
    }
    buffOrder[bufferIdx] = order;   
    //printf("Thread [%llu][%2u] order [%2d], got cmd [%x]\n", std::this_thread::get_id(),thread_id, order, result);
    return result;
}

void oGFX::CommandBufferManager::EndCommandBuffer(uint32_t thread_id, VkCommandBuffer cmd)
{
    auto& submitted = threadSubmitteds[thread_id];

    size_t idx = FindCmdIdx(thread_id, cmd);
    OO_ASSERT(submitted[idx] == eRECSTATUS::RECORDING);
    // end command
    vkEndCommandBuffer(cmd);
    submitted[idx] = eRECSTATUS::ENDED;
}

void oGFX::CommandBufferManager::ResetPools()
{
    for (size_t i = 0; i < MAX_THREADS; i++)
    {
        nextIndices[i] = 0;
        VkCommandPoolResetFlags flags{ VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT };
        VK_CHK(vkResetCommandPool(m_device, m_commandpools[i], flags));
        std::fill(threadSubmitteds[i].begin(), threadSubmitteds[i].end(), eRECSTATUS::INVALID);
        std::fill(threadOrder[i].begin(), threadOrder[i].end(), 0);
    }
}

void oGFX::CommandBufferManager::DestroyPools()
{
    for (size_t i = 0; i < MAX_THREADS; i++)
    {
        if (m_commandpools[i]);
        vkDestroyCommandPool(m_device, m_commandpools[i], nullptr);
        m_commandpools[i] = VK_NULL_HANDLE;
    }    
}

void oGFX::CommandBufferManager::SubmitCommandBuffer(uint32_t thread_id,VkQueue queue, VkCommandBuffer cmd)
{
    size_t idx = FindCmdIdx(thread_id,cmd);

    auto& submitted = threadSubmitteds[thread_id];
    if (submitted[idx] == eRECSTATUS::RECORDING)
    {
        // End commands
        vkEndCommandBuffer(cmd);
    }
    submitted[idx] = eRECSTATUS::SUBMITTED;
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    PROFILE_SCOPED("SUBMIT SINGLE");
    //assuming we have only a few meshes to load we will pause here until we load the previous object
    //submit transfer commands to transfer queue and wait until it finishes
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
}

void oGFX::CommandBufferManager::SubmitCommandBufferAndWait(uint32_t thread_id, VkQueue queue, VkCommandBuffer cmd)
{
    SubmitCommandBuffer(thread_id, queue, cmd);
    vkQueueWaitIdle(queue);
}

void oGFX::CommandBufferManager::SubmitAll(VkQueue queue, VkSubmitInfo inInfo, VkFence signalFence)
{
    // batch together entire submit
    std::vector<std::pair<uint32_t,VkCommandBuffer>> orderedBuffers;
    for (size_t thread_id = 0; thread_id < MAX_THREADS; thread_id++)
    {
        auto& commandBuffers = threadCBs[thread_id];
        auto& submitted = threadSubmitteds[thread_id];
        auto& order = threadOrder[thread_id];
        orderedBuffers.reserve(orderedBuffers.size() + commandBuffers.size());
        for (size_t i = 0; i < submitted.size(); i++)
        {
            if (submitted[i] == eRECSTATUS::RECORDING)
            {
                PROFILE_SCOPED("END CMDBUFFER");
                vkEndCommandBuffer(commandBuffers[i]);
                submitted[i] = eRECSTATUS::ENDED;
            }

            if (submitted[i] == eRECSTATUS::ENDED)
            {
                orderedBuffers.emplace_back(std::make_pair(order[i], commandBuffers[i]));
            }
        }
        for (auto& sub : submitted)
        {
            if (sub == eRECSTATUS::ENDED)
                sub = eRECSTATUS::SUBMITTED;
        }
    }  

    std::vector<VkCommandBuffer> buffers;
    buffers.resize(orderedBuffers.size());

    std::sort(orderedBuffers.begin(), orderedBuffers.end(),
        [](const std::pair<uint32_t, VkCommandBuffer>& l, const std::pair<uint32_t, VkCommandBuffer>& r) {
            return l.first < r.first;
        }
    );
    for (size_t i = 0; i < orderedBuffers.size(); i++)
    {
        buffers[i] = orderedBuffers[i].second;
        //printf("Submitting order [%d] cmd [%p]\n", orderedBuffers[i].first, buffers[i]);
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = buffers.size();
    submitInfo.pCommandBuffers = buffers.data();

    submitInfo.pSignalSemaphores = inInfo.pSignalSemaphores;
    submitInfo.signalSemaphoreCount = inInfo.signalSemaphoreCount;

    submitInfo.pWaitSemaphores= inInfo.pWaitSemaphores;
    submitInfo.waitSemaphoreCount = inInfo.waitSemaphoreCount;
    submitInfo.pWaitDstStageMask = inInfo.pWaitDstStageMask;

    submitInfo.pNext = inInfo.pNext;
    {
        PROFILE_SCOPED("SUBMIT QUEUE");
        vkQueueSubmit(queue, 1, &submitInfo, signalFence);
    }

}

size_t oGFX::CommandBufferManager::FindCmdIdx(uint32_t thread_id, VkCommandBuffer cmd)
{
    auto& commandBuffers = threadCBs[thread_id];

    auto iter = std::find(commandBuffers.begin(), commandBuffers.end(), cmd);
    OO_ASSERT(iter != commandBuffers.end() && "invalid usage");
    auto idx = iter - commandBuffers.begin();

    return idx;
}

void oGFX::CommandBufferManager::AllocateCommandBuffer(uint32_t thread_id)
{
    //std::cout << __FUNCTION__ << std::endl;
	VkCommandBufferAllocateInfo cbAllocInfo = {};
	cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;	// VK_COMMAND_BUFFER_LEVEL_PRIMARY : buffer you submit directly to queue, cant be called  by other buffers
	//VK_COMMAND_BUFFER_LEVEL_SECONDARY :  buffer cant be called directly, can be called from other buffers via "vkCmdExecuteCommands" when recording commands in primary buffer
	cbAllocInfo.commandBufferCount = 1;
	cbAllocInfo.commandPool = m_commandpools[thread_id];

	VkCommandBuffer cb;
	VkResult result = vkAllocateCommandBuffers(m_device, &cbAllocInfo, &cb);
	if (result != VK_SUCCESS)
	{
		std::cerr << "Failed to allocate Command Buffers!" << std::endl;
		__debugbreak();
	}

    threadCBs[thread_id].emplace_back(cb);
    threadSubmitteds[thread_id].emplace_back(eRECSTATUS::INVALID);
    threadOrder[thread_id].emplace_back(0);
}

