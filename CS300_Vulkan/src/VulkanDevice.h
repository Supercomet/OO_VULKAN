#pragma once

#include <vulkan/vulkan.h>
#include "VulkanUtils.h"

struct Window;
struct VulkanInstance;
struct VulkanDevice
{
	~VulkanDevice();
	void InitPhysicalDevice(VulkanInstance& instance);
	void InitLogicalDevice(VulkanInstance& instance);
	

	friend class VulkanRenderer;
	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;
	VulkanInstance* m_instancePtr;

	VkQueue graphicsQueue;
	VkQueue presentationQueue;
	bool CheckDeviceSuitable(VkPhysicalDevice device);
	
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

};

