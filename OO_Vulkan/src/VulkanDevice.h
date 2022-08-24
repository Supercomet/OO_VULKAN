#pragma once

#include <vulkan/vulkan.h>
#include "VulkanUtils.h"
#include "VulkanBuffer.h"

struct Window;
struct VulkanInstance;
struct VulkanDevice
{
	~VulkanDevice();
	VulkanDevice(const VulkanDevice&) = delete;

	void InitPhysicalDevice(VulkanInstance& instance);
	void InitLogicalDevice(VulkanInstance& instance);
	

	friend class VulkanRenderer;
	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;
	VulkanInstance* m_instancePtr;

	VkQueue graphicsQueue;
	oGFX::QueueFamilyIndices queueIndices;
	VkQueue presentationQueue;

	VkPhysicalDeviceFeatures enabledFeatures;
	VkPhysicalDeviceProperties properties;

	VkCommandPool commandPool = VK_NULL_HANDLE;

	bool CheckDeviceSuitable(VkPhysicalDevice device);
	
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

	VkResult CreateBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags,
		vkutils::Buffer* buffer, VkDeviceSize size, void* data = nullptr);

	void CopyBuffer(vkutils::Buffer* src, vkutils::Buffer* dst, VkQueue queue,
			VkBufferCopy* copyRegion = nullptr);

	VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin = false);
	VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level, bool begin = false);
	void FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free = true);
	void FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free = true);

};

