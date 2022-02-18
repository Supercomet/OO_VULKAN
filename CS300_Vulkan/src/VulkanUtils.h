#pragma once

#include <vulkan/vulkan.h>

#include <vector>
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
}

