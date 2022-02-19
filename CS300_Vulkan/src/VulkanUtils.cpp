#define NOMINMAX

#if defined(_WIN32)
#include <windows.h>
#endif

#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <vector>

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

	VkFormat ChooseSupportedFormat(VulkanDevice& device, const std::vector<VkFormat> &formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
	{
		// loop through optpions and find compatible one
		for (VkFormat format : formats)
		{
			//get properties for given format on this device
			VkFormatProperties properties;
			vkGetPhysicalDeviceFormatProperties(device.physicalDevice, format, &properties);

			//depending on tilting choice, check the bit flag
			if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featureFlags) == featureFlags)
			{
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featureFlags) == featureFlags)
			{
				return format;
			}
		}

		throw std::runtime_error("Failed to find a matching format!");
	}

	VkImage CreateImage(VulkanDevice& device,uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory *imageMemory)
	{
		// Create image
		// Image creation info
		VkImageCreateInfo imageCreateInfo{};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;				//type of image, (1D, 2D ,3D)
		imageCreateInfo.extent.width = width;						// width of image extents
		imageCreateInfo.extent.height = height;						// height of image extents
		imageCreateInfo.extent.depth = 1;							// depth of image (just 1, no 3d)
		imageCreateInfo.mipLevels = 1;								// Number of mipmap levels
		imageCreateInfo.arrayLayers = 1;							// number of leves in image array
		imageCreateInfo.format = format;							// Format type of image
		imageCreateInfo.tiling = tiling;							// How image data should be "tiled"
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;	// layout of image data on creation
		imageCreateInfo.usage = useFlags;							// Bit flags definining wha timage will be used for
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;			// number of samples for multisampling
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	// Whether image can be shared between queues
		VkImage image;
		VkResult result = vkCreateImage(device.logicalDevice, &imageCreateInfo, nullptr, &image);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create an image!");
		}
		// Create memory for iamge
		//get memeory requirements for a type of image
		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(device.logicalDevice, image, &memoryRequirements);

		// allocate memory using image requirements and user defined properties
		VkMemoryAllocateInfo memoryAllocInfo{};
		memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocInfo.allocationSize = memoryRequirements.size;
		memoryAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(device.physicalDevice,memoryRequirements.memoryTypeBits,propFlags);

		result = vkAllocateMemory(device.logicalDevice, &memoryAllocInfo, nullptr, imageMemory);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed allocate memory for an image!");
		}

		// connect memory to image
		vkBindImageMemory(device.logicalDevice, image, *imageMemory, 0);

		return image;
	}

	uint32_t FindMemoryTypeIndex(VkPhysicalDevice physicalDevice,uint32_t allowedTypes, VkMemoryPropertyFlags properties)
	{
		//get properties of my physical device memory
		VkPhysicalDeviceMemoryProperties memoryproperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryproperties);

		for (uint32_t i = 0; i < memoryproperties.memoryTypeCount; i++)
		{
			if ((allowedTypes & (1 << i)) //index of memory type must match correspoding bit in allowedTypes
				&& (memoryproperties.memoryTypes[i].propertyFlags & properties) == properties) // desired property bit flags are part of  memory type's properties flags
			{
				//this memory type is valid so return its index
				return i;
			}
		}
	}

	VkShaderModule CreateShaderModule(VulkanDevice& device,const std::vector<char> &code)
	{
		// Shader module creation information
		VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
		shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		//size of code
		shaderModuleCreateInfo.codeSize = code.size();
		//pointer to code (cast to uint32_t)
		shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

		VkShaderModule shaderModule;
		VkResult result = vkCreateShaderModule(device.logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a shader module!");
		}
		return shaderModule;
	}

	std::vector<char> readFile(const std::string &filename)
	{
		//open stream from given file
		//std::ios:binary , tells stream to read file as binary
		//std::ios::ate , tells stream to start reading from the end of file
		std::ifstream file(filename, std::ios::binary | std::ios::ate);

		//check if file stream success fully opened
		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open a file!");
		}

		// get current read position and use to resize buffer
		size_t fileSize = (size_t)file.tellg();
		std::vector<char> fileBuffer(fileSize);

		//move cursor to start of file
		file.seekg(0);

		//read data into the buffer
		file.read(fileBuffer.data(),fileSize);

		//close the stream
		file.close();

		return fileBuffer;
	}

	void CreateBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags bufferProperties, VkBuffer* buffer, VkDeviceMemory* bufferMemory)
	{
		//CREATE VERTEX BUFFER
		//information to create a buffer ( doesnt include assigning memory)
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size =bufferSize;							// size of buffer (size of 1 vertex pos * number of verts)
		bufferInfo.usage = bufferUsage;							//multiple types of buffer possible
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;		// similar to swapchain images , we can share vertex buffers

		VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, buffer);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create a Vertex Buffer!");
		}

		//get buffer memory requirements
		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(device, *buffer, &memoryRequirements);

		// Allocate memory to buffer
		VkMemoryAllocateInfo memoryAllocInfo = {};
		memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocInfo.allocationSize = memoryRequirements.size;
		memoryAllocInfo.memoryTypeIndex = FindMemoryTypeIndex(physicalDevice,memoryRequirements.memoryTypeBits,								//index of memory type on physical device that has required bit flags
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);	// VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : CPU can interact with memory
																							// VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : Allows placement of data straight into buffer after mapping (otherwise would have to specify manually)
																							//Allocate memory to VkDeviceMemory
		result = vkAllocateMemory(device, &memoryAllocInfo, nullptr, bufferMemory);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate Vertex Buffer Memory!");
		}

		//Allocate memory to given vertex buffer
		vkBindBufferMemory(device, *buffer, *bufferMemory, 0);
	}


}
