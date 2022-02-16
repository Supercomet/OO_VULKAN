#pragma once

#include <vulkan/vulkan.h>

class Window;

class VulkanDevice
{
public:
	void Init(Window& window);
private:
	friend class VulkanRenderer;
	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;
};

