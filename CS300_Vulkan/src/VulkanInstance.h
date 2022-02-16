#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "gpuCommon.h"


class VulkanInstance
{
public:
	VulkanInstance() = default;
	~VulkanInstance();
	bool Init(const oGFX::SetupInfo& si);
private:
	friend class VulkanRenderer;
	VkInstance instance{VK_NULL_HANDLE};
};

