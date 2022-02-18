#define NOMINMAX

#if defined(_WIN32)
#include <windows.h>
#endif

#include <algorithm>
#include <stdexcept>

#include "VulkanUtils.h"
#include "VulkanInstance.h"
#include "VulkanDevice.h"
namespace oGFX
{
	oGFX::QueueFamilyIndices GetQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
	{

		oGFX::QueueFamilyIndices indices;

		//get the queue family properties for the device
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

		//go through each queue family and check if it has at least one of the required types of queue
		// it is possible to have a graphics queue family with no queues in it
		int i = 0;
		for (const auto& queueFamily : queueFamilyList)
		{
			// is there at least one queue in the queue family? 
			// queues can be multiple types, bitwise and VK_QUEUE and type to check it has required type
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				//found queue family get the index
				indices.graphicsFamily = i;
			}

			//check if queue family supports presentation
			VkBool32 presentationSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);
			//check if queue is presentation type ( can be both graphics and presentation)
			if (queueFamily.queueCount > 0 && presentationSupport)
			{
				indices.presentationFamily = i;
			}

			// check if queue family indeces are in a valid state, stop searching
			if (indices.isValid())
			{
				break;
			}
			i++;
		}

		return indices;
	}	   

    oGFX::SwapChainDetails GetSwapchainDetails(VulkanInstance& instance, VkPhysicalDevice device)
    {
        oGFX::SwapChainDetails swapChainDetails;

        //CAPABILITIES
        //get the surface capabilities for the given surface on the given physical device
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, instance.surface , &swapChainDetails.surfaceCapabilities);

        // FORMATS
        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, instance.surface, &formatCount, nullptr);

        //if formats returned get the list of formats
        if (formatCount != 0)
        {
            swapChainDetails.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, instance.surface, &formatCount, swapChainDetails.formats.data());
        }

        // PRESENTATION MODES
        uint32_t presentationCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, instance.surface, &presentationCount, nullptr);

        //if presentation modes returned get list of presentation modes.
        if (presentationCount != 0)
        {
            swapChainDetails.presentationModes.resize(presentationCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, instance.surface, &presentationCount, swapChainDetails.presentationModes.data());
        }

        return swapChainDetails;
    }

	VkSurfaceFormatKHR ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
	{
		//If only 1 format available and is VK_FORMAT_UNDEFINED return what format we want.
		// VK_FORMAT_UNDEFINED - means that all formats are available (no restrictions) so we can return what we want
		if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
		{
			return { VK_FORMAT_R8G8B8A8_UNORM , VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}

		//If we are restricted, search for optimal format
		//Could write proper algorithm for finding best colorspace and color format combination
		for (const auto &format : formats)
		{
			if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM) && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return format;
			}
		}

		// we just return the first format and hope for the best
		return formats[0];
	}

	VkPresentModeKHR ChooseBestPresentationMode(const std::vector<VkPresentModeKHR> presentationModes)
	{
		//look for mailbox presentaiton mode
		for (const auto &presentationMode : presentationModes)
		{
			if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return presentationMode;
			}
		}
		//return most default case, this SHOULD always be availabe according to Vulkan spec
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
	{
		// if current extent is at numeric limits, then extent can vary, otherwise it is the size of the window.
		if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return surfaceCapabilities.currentExtent;
		}
		else
		{
			//if value can vary, need to set manually

			//get window size
			int width, height;
			

			width = GetSystemMetrics(SM_CXSCREEN);
			height = GetSystemMetrics(SM_CYSCREEN);
			//glfwGetFramebufferSize(window, &width, &height);

			//create new extent using window size
			VkExtent2D newExtent = {};
			newExtent.width = static_cast<uint32_t>(width);
			newExtent.height = static_cast<uint32_t>(height);

			//surface also defines max and min
			//make sure without bounds by clamping
			newExtent.width = std::max(surfaceCapabilities.minImageExtent.width, std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width));
			newExtent.height = std::max(surfaceCapabilities.minImageExtent.height, std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height));

			return newExtent;
		}
	}

	VkImageView CreateImageView(VulkanDevice& device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
	{
		VkImageViewCreateInfo viewCreateInfo = {};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.image = image;
		//type of image  (1d 2d 3d cubemap etc..)
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.format = format;
		//allows remapping of RBGA via swizzle
		viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		//subresources allow the view to view only a part of an image
		//which aspect of image to view (color bit, depth stencil etc)
		viewCreateInfo.subresourceRange.aspectMask = aspectFlags;
		//start mipmap level to view from
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		//number of mipmap levels to view
		viewCreateInfo.subresourceRange.levelCount = 1;
		//start of array level to view from
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		//number of array levels to view
		viewCreateInfo.subresourceRange.layerCount = 1;

		//create image view and return it
		VkImageView imageView;
		VkResult result = vkCreateImageView(device.logicalDevice, &viewCreateInfo, nullptr, &imageView);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create an image view!");
		}

		return imageView;
	}
}
