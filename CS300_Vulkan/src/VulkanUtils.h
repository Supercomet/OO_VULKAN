#pragma once

#include <vulkan/vulkan.h>

#include <vector>
struct VulkanInstance;
struct VulkanDevice;
namespace oGFX
{
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

	struct SwapChainImage
	{
		VkImage image;
		VkImageView imageView;
	};

	oGFX::SwapChainDetails GetSwapchainDetails(VulkanInstance& instance,VkPhysicalDevice device);
	oGFX::QueueFamilyIndices GetQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

	VkSurfaceFormatKHR ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR ChooseBestPresentationMode(const std::vector<VkPresentModeKHR> presentationModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);

	VkImageView CreateImageView(VulkanDevice& device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

}

