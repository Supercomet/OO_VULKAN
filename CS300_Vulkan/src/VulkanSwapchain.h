#pragma once
#include "VulkanUtils.h"

struct VulkanDevice;
struct oGFX::SwapChainImage;
struct VulkanSwapchain
{
	~VulkanSwapchain();
	void Init(VulkanInstance& instance,VulkanDevice& device);

	VkSwapchainKHR swapchain;
	std::vector<oGFX::SwapChainImage> swapChainImages;
	uint32_t minImageCount;

	//utility
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	VkFormat depthFormat;

	VulkanDevice* m_devicePtr;

};

