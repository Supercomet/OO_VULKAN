#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "VulkanInstance.h"
#include "gpuCommon.h"


class VulkanRenderer
{
public:
	void CreateInstance(const oGFX::SetupInfo& setupSpecs);
private:
	VulkanInstance m_instance;
};

