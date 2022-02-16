#pragma once

#include <vector>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "gpuCommon.h"

class Window;

class VulkanRenderer
{
public:
	void CreateInstance(const oGFX::SetupInfo& setupSpecs);
	void CreateSurface(const Window& window);
	void AcquirePhysicalDevice();
private:
	VulkanInstance m_instance;
	VulkanDevice m_device;
	VkSurfaceKHR m_surface;

	// Indices (locations) of Queue Familities (if they exist)
	struct QueueFamilyIndices
	{
		int graphicsFamily = -1; //location of graphics queue family //as per vulkan standard, if we have a graphics family, we have a transfer family
		int presentationFamily = -1;

		//check if queue familities are valid
		bool isValid()
		{
			return graphicsFamily >= 0 && presentationFamily >=0;
		}
	};

	struct SwapChainDetails
	{
		//surfaces properties , image sizes/extents etc...
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		//surface image formats, eg. RGBA, data size of each color
		std::vector<VkSurfaceFormatKHR> formats;
		//how images should be presentated to screen, filo fifo etc..
		std::vector<VkPresentModeKHR> presentationModes;
		SwapChainDetails() :surfaceCapabilities{} {}
	};

	bool CheckDeviceSuitable(VkPhysicalDevice device);
	QueueFamilyIndices GetQueueFamilies(VkPhysicalDevice device);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
	SwapChainDetails GetSwapchainDetails(VkPhysicalDevice device);
};

