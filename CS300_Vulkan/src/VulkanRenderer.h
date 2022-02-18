#pragma once

#include <vector>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "gpuCommon.h"

struct Window;

class VulkanRenderer
{
public:
	~VulkanRenderer();

	void CreateInstance(const oGFX::SetupInfo& setupSpecs);
	void CreateSurface(Window& window);
	void AcquirePhysicalDevice();
	void CreateLogicalDevice();
	void SetupSwapchain();

	VulkanInstance m_instance;
	VulkanDevice m_device;
	VulkanSwapchain m_swapchain;

	Window* windowPtr;


};

