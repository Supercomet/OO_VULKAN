/************************************************************************************//*!
\file           VulkanUtils.cpp
\project        Ouroboros
\author         Jamie Kong, j.kong, 390004720 | code contribution (100%)
\par            email: j.kong\@digipen.edu
\date           Oct 02, 2022
\brief          Utility function definitions

Copyright (C) 2022 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*//*************************************************************************************/

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "MathCommon.h"

#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <vector>


#pragma warning( push ,0 )
#include "loader/stb_image.h"
#include "loader/DDSLoader.h"
#pragma warning( pop )
#include <filesystem>


#include "VulkanUtils.h"
#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanRenderer.h" // pfn

#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <vector>
#include <filesystem>

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
		//reset
		i = 0;
		for (const auto& queueFamily : queueFamilyList)
		{
			if (i == indices.graphicsFamily)
			{
				i++;
				continue;
			}

			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				//found queue family get the index
				indices.transferFamily = i;
				break;
			}
			i++;
		}

		indices.transferFamily = indices.transferFamily < 0 ? indices.graphicsFamily : indices.transferFamily;
		
		return indices;
	}	   

	//glm::mat4 ortho(float aspect_ratio, float size, float nr, float fr)
	//{
	//	
	//		glm::mat4 mat;
	//
	//		//float top = size * 0.5f;
	//		float top = size;
	//		float bottom = -top;
	//		float right = top * aspect_ratio;
	//		float left = bottom * aspect_ratio;
	//
	//		mat[0*4 +0] = 2.0f / (right - left);
	//		mat[1*4 +1] = 2.0f / (top - bottom);
	//		mat[2*4 +2] = 1.0f / (fr - nr);
	//
	//		//this was missing..
	//		mat.m[3*4+ 0] = -(right + left) / (right - left);
	//		mat.m[3*4+ 1] = -(top + bottom) / (top - bottom);
	//		mat.m[3*4+ 2] = fr/(fr-nr);
	//
	//		return mat;
	//	
	//}

	glm::mat4 customOrtho(float aspect_ratio, float size, float nr, float fr)
	{
			glm::mat4 mat;
	
			//float top = size * 0.5f;
			float top = size;
			float bottom = -top;
			float right = top * aspect_ratio;
			float left = bottom * aspect_ratio;
	
			mat[0 ][0] = 2.0f / (right - left);
			mat[1 ][1] = 2.0f / (top - bottom);
			mat[2 ][2] = 1.0f / (fr - nr);
	
			//this was missing..
			mat[3][ 0] = -(right + left) / (right - left);
			mat[3][ 1] = -(top + bottom) / (top - bottom);
			mat[3][ 2] = fr/(fr-nr);
	
			return mat;
	}

	const std::vector<VkVertexInputBindingDescription>& GetGFXVertexInputBindings()
	{
	
		static std::vector<VkVertexInputBindingDescription> bindingDescription {	
			oGFX::vkutils::inits::vertexInputBindingDescription(BIND_POINT_VERTEX_BUFFER_ID,sizeof(Vertex),VK_VERTEX_INPUT_RATE_VERTEX),
			//oGFX::vkutils::inits::vertexInputBindingDescription(BIND_POINT_INSTANCE_BUFFER_ID,sizeof(oGFX::InstanceData),VK_VERTEX_INPUT_RATE_INSTANCE),
		};
		return bindingDescription;
	
	}

	const std::vector<VkVertexInputAttributeDescription>& GetGFXVertexInputAttributes()
	{
		static std::vector<VkVertexInputAttributeDescription>attributeDescriptions{
		oGFX::vkutils::inits::vertexInputAttributeDescription(BIND_POINT_VERTEX_BUFFER_ID,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex, pos)), //Position attribute
		oGFX::vkutils::inits::vertexInputAttributeDescription(BIND_POINT_VERTEX_BUFFER_ID,1,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex, norm)),//normals attribute
		oGFX::vkutils::inits::vertexInputAttributeDescription(BIND_POINT_VERTEX_BUFFER_ID,2,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex, col)), // colour attribute
		oGFX::vkutils::inits::vertexInputAttributeDescription(BIND_POINT_VERTEX_BUFFER_ID,3,VK_FORMAT_R32G32B32_SFLOAT,offsetof(Vertex, tangent)),//tangent attribute
		oGFX::vkutils::inits::vertexInputAttributeDescription(BIND_POINT_VERTEX_BUFFER_ID,4,VK_FORMAT_R32G32_SFLOAT	  ,offsetof(Vertex, tex)),    //Texture attribute
	
		// instance data attributes
		//oGFX::vkutils::inits::vertexInputAttributeDescription(BIND_POINT_INSTANCE_BUFFER_ID,15,VK_FORMAT_R32G32B32A32_UINT,offsetof(InstanceData, InstanceData::instanceAttributes)),
		};
		return attributeDescriptions;
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
		//return { VK_FORMAT_B8G8R8A8_SRGB,  VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		
		//If only 1 format available and is VK_FORMAT_UNDEFINED return what format we want.
		// VK_FORMAT_UNDEFINED - means that all formats are available (no restrictions) so we can return what we want
		if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
		{
			return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
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
		VK_NAME(device.logicalDevice, "createImageView::imageView", imageView);
		if (result != VK_SUCCESS)
		{
			std::cerr << "Failed to create an image view!" << std::endl;
			__debugbreak();
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

		std::cerr << "Failed to find a matching format!" << std::endl;
		__debugbreak();
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
		VK_NAME(device.logicalDevice, "CreateImage::image", image);
		if (result != VK_SUCCESS)
		{
			std::cerr << "Failed to create an image!" << std::endl;
			__debugbreak();
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
			std::cerr << "Failed allocate memory for an image!" << std::endl;
			__debugbreak();
		}

		// connect memory to image
		vkBindImageMemory(device.logicalDevice, image, *imageMemory, 0);

		return image;
	}

	uint32_t FindMemoryTypeIndex(VkPhysicalDevice physicalDevice,uint32_t allowedTypes, VkMemoryPropertyFlags properties)
	{
		//get properties of my physical device memory
		VkPhysicalDeviceMemoryProperties memoryproperties;
		// VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : CPU can interact with memory
		// VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : Allows placement of data straight into buffer after mapping (otherwise would have to specify manually)
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
		return static_cast<uint32_t>(-1);
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
		VK_NAME(device.logicalDevice, code.data(), shaderModule);
		if (result != VK_SUCCESS)
		{
			std::cerr << "Failed to create a shader module!" << std::endl;
			__debugbreak();
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
			std::cerr << "Failed to open a file! : " << filename << std::endl;
			__debugbreak();
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

	void CreateBuffer(const std::string& name, VmaAllocator allocator, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage, VmaAllocationCreateFlags allocationFlags, oGFX::AllocatedBuffer& vmabuffer)
	{
		CreateBuffer(allocator, bufferSize, bufferUsage, allocationFlags, vmabuffer);
		vmabuffer.name = name;
		VK_NAME(VulkanRenderer::get()->m_device.logicalDevice, vmabuffer.name.c_str() , vmabuffer.buffer);
	}

	void CreateBuffer(VmaAllocator allocator, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage,
		VmaAllocationCreateFlags allocationFlags, oGFX::AllocatedBuffer& vmabuffer)
	{
		PROFILE_SCOPED();
		//CREATE VERTEX BUFFER
		//information to create a buffer ( doesnt include assigning memory)
		VmaAllocationCreateInfo vmaCI{};
		vmaCI.usage = VMA_MEMORY_USAGE_AUTO;
		vmaCI.flags = allocationFlags;
		VkBufferCreateInfo bufferInfo = oGFX::vkutils::inits::bufferCreateInfo(bufferUsage,bufferSize);
		VkResult result = vmaCreateBuffer(allocator, &bufferInfo, &vmaCI, &vmabuffer.buffer, &vmabuffer.alloc, &vmabuffer.allocInfo);
		if (result != VK_SUCCESS)
		{
			std::cerr << "Failed to create a Buffer!" << std::endl;
			__debugbreak();
		}
		vmabuffer.dbi.buffer = vmabuffer.buffer;
		vmabuffer.dbi.range = VK_WHOLE_SIZE;
	}

	void CreateOrResizeBuffer(VmaAllocator allocator, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage, VmaAllocationCreateFlags allocationFlags, oGFX::AllocatedBuffer& vmabuffer)
	{
		if (bufferSize <= vmabuffer.allocInfo.size) return;

		if (vmabuffer.buffer) {
			vmaDestroyBuffer(allocator, vmabuffer.buffer, vmabuffer.alloc);
		}

		VmaAllocationCreateInfo vmaCI{};
		vmaCI.usage = VMA_MEMORY_USAGE_AUTO;
		vmaCI.flags = allocationFlags;
		VkBufferCreateInfo bufferInfo = oGFX::vkutils::inits::bufferCreateInfo(bufferUsage, bufferSize);
		VkResult result = vmaCreateBuffer(allocator, &bufferInfo, &vmaCI, &vmabuffer.buffer, &vmabuffer.alloc, &vmabuffer.allocInfo);
		if (result != VK_SUCCESS)
		{
			std::cerr << "Failed to create a Buffer!" << std::endl;
			__debugbreak();
		}
		vmabuffer.dbi.buffer = vmabuffer.buffer;
		vmabuffer.dbi.range = VK_WHOLE_SIZE;
	}

	// TODO: Allow this to make multiple segmented copies
	void CopyBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool,
		VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize,VkDeviceSize dstOffset,VkDeviceSize srcOffset)
	{
		PROFILE_SCOPED();
		//Create buffer
		VkCommandBuffer transferCommandBuffer = beginCommandBuffer(device,transferCommandPool);

		// region of data to copy from and to
		VkBufferCopy bufferCopyRegion{};
		bufferCopyRegion.srcOffset = srcOffset;
		bufferCopyRegion.dstOffset = dstOffset;
		bufferCopyRegion.size = bufferSize;

		// command to copy src buffer to dst buffer
		vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);

		endAndSubmitCommandBuffer(device, transferCommandPool, transferQueue, transferCommandBuffer);
	}

	void CopyBuffer(VkCommandBuffer cmd, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize, VkDeviceSize dstOffset, VkDeviceSize srcOffset)
	{
		// region of data to copy from and to
		VkBufferCopy bufferCopyRegion{};
		bufferCopyRegion.srcOffset = srcOffset;
		bufferCopyRegion.dstOffset = dstOffset;
		bufferCopyRegion.size = bufferSize;

		// command to copy src buffer to dst buffer
		vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &bufferCopyRegion);
	}

	VkCommandBuffer beginCommandBuffer(VkDevice device, VkCommandPool commandPool)
	{

		//command buffer to hold transfer commands
		VkCommandBuffer commandBuffer;

		//command buffer details
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		//allocate command buffer from pool
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
		//std::cout << " Begin comand buffer 2 " << commandBuffer << "\n";

		//information to begin the command buffer record.
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // only using the command buffer once, so its a one time submit

																	   //begin recording transfer commands
		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void endAndSubmitCommandBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer)

	{
		//std::cout << " End comand buffer 2 " << commandBuffer << "\n";
		// End commands
		vkEndCommandBuffer(commandBuffer);

		//queue submission information
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		//assuming we have only a few meshes to load we will pause here until we load the previous object
		//submit transfer commands to transfer queue and wait until it finishes
		vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(queue);

		// Free temporary command buffer to pool
		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
		//std::cout << " Free comand buffer 2 " << commandBuffer << "\n";
	}

	void CopyImageBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool, VkBuffer srcBuffer, VkImage image, uint32_t width, uint32_t height)
	{
		//Create buffer
		VkCommandBuffer transferCommandBuffer = beginCommandBuffer(device, transferCommandPool);

		VkBufferImageCopy imageRegion{};
		imageRegion.bufferOffset = 0;											// Offset into data
		imageRegion.bufferRowLength = 0;										// Row length of data to calculate data spacing
		imageRegion.bufferImageHeight = 0;										// Image height to calculate data spacing
		imageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;	// Which aspect of image to copy
		imageRegion.imageSubresource.mipLevel = 0;								// Mipmap Level to copy
		imageRegion.imageSubresource.baseArrayLayer = 0;						// Starting array layer (if array)
		imageRegion.imageSubresource.layerCount = 1;							// Number of layers to copy starting at baseArray layer
		imageRegion.imageOffset = { 0,0,0 };									//	Offset into image (as opposed to raw data in buffer offset)
		imageRegion.imageExtent = { width, height, 1 };							//  Size of region to copy as XYZ values

																				// Copy buffer to given image
		vkCmdCopyBufferToImage(transferCommandBuffer, srcBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageRegion);

		endAndSubmitCommandBuffer(device, transferCommandPool, transferQueue, transferCommandBuffer);
	}


	void FreeTextureFile(uint8_t* data)
	{
		stbi_image_free(data);
	}

	

	bool IsFileDDS(const std::string& fileName)
	{
		auto path = std::filesystem::path(fileName);
		std::string a = path.extension().string();
		std::string b(".dds");
		
		return std::equal(a.begin(), a.end(),
			b.begin(), b.end(),
			[](char a, char b) {
				return tolower(a) == b;
			});
	}

	bool IsFileHDR(const std::string& fileName)
	{
		auto path = std::filesystem::path(fileName);
		std::string a = path.extension().string();
		std::string b(".hdr");

		return std::equal(a.begin(), a.end(),
			b.begin(), b.end(),
			[](char a, char b) {
				return tolower(a) == b;
			});
	}

	bool FileImageData::Create(const std::string& fileName)
	{
		name = fileName;
		if (oGFX::IsFileDDS(fileName) == true)
		{
			decodeType = ExtensionType::DDS;
			//imgData = LoadDDS(fileName, &this->w, &this->h, &this->channels, &this->dataSize);
			LoadDDS(fileName, *this);
			generateMips = mipInformation.size() > 1;
			return imgData.size() ? true : false;
		}
		else
		{
			decodeType = ExtensionType::STB;
			auto ptr = stbi_load(fileName.c_str(), &this->w, &this->h, &this->channels, STBI_rgb_alpha);
			dataSize = size_t(this->w) * size_t(this->h) * size_t(STBI_rgb_alpha);
			imgData.resize(dataSize);
			memcpy(imgData.data(), ptr, dataSize);
			stbi_image_free(ptr);

			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = 0;
			bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = this->w;
			bufferCopyRegion.imageExtent.height = this->h;
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = 0;
			mipInformation.push_back(bufferCopyRegion);

			generateMips = true;
			this->format = VK_FORMAT_R8G8B8A8_UNORM;
			return imgData.size() ? true : false;
		}

		return false;
	}


	uint16_t float_to_half(const float x)
	{
		const uint32_t HALF_FLOAT_MAX_VALUE = 65504;
		const uint32_t HALF_FLOAT_MAX = 0x7bff;

		if (x > HALF_FLOAT_MAX_VALUE) return uint16_t(HALF_FLOAT_MAX);// guard against high values
		// IEEE-754 16-bit floating-point format (without infinity): 1-5-10, exp-15, +-131008.0, +-6.1035156E-5, +-5.9604645E-8, 3.311 digits
		const uint32_t b = *reinterpret_cast<const uint32_t*>(&x) + 0x00001000; // round-to-nearest-even: add last bit after truncated mantissa
		const uint32_t e = (b & 0x7F800000) >> 23; // exponent
		const uint32_t m = b & 0x007FFFFF; // mantissa; in line below: 0x007FF000 = 0x00800000-0x00001000 = decimal indicator flag - initial rounding
		return (b & 0x80000000) >> 16 | (e > 112) * ((((e - 112) << 10) & 0x7C00) | m >> 13) | ((e < 113) & (e > 101)) * ((((0x007FF000 + m) >> (125 - e)) + 1) >> 1) | (e > 143) * 0x7FFF; // sign : normalized : denormalized : saturate
	}

	bool FileImageData::CreateCube(const std::string& folder)
	{
		const uint32_t CUBE_FACES = 6;
		std::array<std::string, CUBE_FACES> fileNames
		{
			"px",
			"nx",
			"py",
			"ny",
			"pz",
			"nz",
		};

		for (const auto& entry : std::filesystem::directory_iterator(folder)) {
			if (entry.is_regular_file()) 
			{
				// Get the file's extension (if any)
				std::string fileName = entry.path().stem().string();
				std::string extension = entry.path().extension().string();

				for (size_t i = 0; i < fileNames.size(); i++)
				{
					if (fileNames[i] == fileName) 
					{
						fileNames[i] = entry.path().string();
					}
				}
			}
		}

		for (size_t i = 0; i < fileNames.size(); i++)
		{
			if (std::filesystem::exists(fileNames[i]) == false) 
			{
				__debugbreak();
			}
		}

		dataSize = 0;
		bool isdds = oGFX::IsFileDDS(fileNames[0]);
		if (isdds == true)
		{
			__debugbreak(); // not implemented
		}
		else
		{
			bool isHDR = IsFileHDR(fileNames.front());
			bool force4Channel = false;
			for (size_t i = 0; i < CUBE_FACES; i++)
			{
				decodeType = ExtensionType::STB;				

				unsigned char* ptr = nullptr;
				size_t chunkSize{};
				if (isHDR) 
				{
					ptr = (unsigned char*)stbi_loadf(fileNames[i].c_str(), &this->w, &this->h, &this->channels, force4Channel? STBI_rgb_alpha :STBI_default);
					if (this->channels == 3) {
						force4Channel = true;
						stbi_image_free(ptr);
						ptr = (unsigned char*)stbi_loadf(fileNames[i].c_str(), &this->w, &this->h, &this->channels, force4Channel ? STBI_rgb_alpha : STBI_default);
					}
					chunkSize = size_t(this->w) * size_t(this->h) * size_t(force4Channel? STBI_rgb_alpha* sizeof(float) : this->channels * sizeof(float));
				}
				else 
				{
					ptr = stbi_load(fileNames[i].c_str(), &this->w, &this->h, &this->channels, STBI_rgb_alpha);
					chunkSize = size_t(this->w) * size_t(this->h) * size_t(STBI_rgb_alpha);
				}

				imgData.resize(dataSize + chunkSize);
				memcpy(imgData.data()+dataSize, ptr, chunkSize);

				VkBufferImageCopy bufferCopyRegion = {};
				bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				bufferCopyRegion.imageSubresource.mipLevel = 0;
				bufferCopyRegion.imageSubresource.baseArrayLayer = (uint32_t)i;
				bufferCopyRegion.imageSubresource.layerCount = 1;
				bufferCopyRegion.imageExtent.width = this->w;
				bufferCopyRegion.imageExtent.height = this->h;
				bufferCopyRegion.imageExtent.depth = 1;
				bufferCopyRegion.bufferOffset = dataSize;
				mipInformation.push_back(bufferCopyRegion);

				dataSize += chunkSize;
				
				stbi_image_free(ptr);
			}

			if (isHDR) 
			{
				this->format = [channels = this->channels]() {
					switch (channels)
					{
					case 1: return VK_FORMAT_R16_SFLOAT;
					case 2: return VK_FORMAT_R16G16_SFLOAT;
					case 3: return VK_FORMAT_R16G16B16A16_SFLOAT;
					case 4: return VK_FORMAT_R16G16B16A16_SFLOAT;
					default:return VK_FORMAT_R16G16B16A16_SFLOAT;
					}
				}();
				std::vector<uint8_t> processed;
				processed.resize(imgData.size() / 2);
				uint16_t* data = (uint16_t*)processed.data();
				for (size_t i = 0; i < processed.size()/2; i++)
				{
					float fltValue = *(float*)(imgData.data() + i * sizeof(float));
					this->highestColValue = std::max(this->highestColValue, fltValue);
					data[i] = float_to_half(fltValue);
				}
				std::swap(imgData, processed);
				dataSize /= 2; // data is halved
				for (size_t i = 0; i < mipInformation.size(); i++)
				{
					mipInformation[i].bufferOffset /= 2;
				}
			}
			else 
			{
				this->format = VK_FORMAT_R8G8B8A8_UNORM;
			}
			return imgData.size() ? true : false;
		}

		return false;
	}

	void FileImageData::Free()
	{
		//if (decodeType == ExtensionType::DDS)
		//{
		//	delete[] imgData;
		//}
		//if (decodeType == ExtensionType::STB)
		//{
		//	stbi_image_free(imgData);
		//}
	}

	void SetVulkanObjectName(VkDevice device,const VkDebugMarkerObjectNameInfoEXT& info)
	{
		auto& vr = *VulkanRenderer::get();
		if (vr.pfnDebugMarkerSetObjectName)
		{
			vr.pfnDebugMarkerSetObjectName(device, &info);
		}
	}

	VkDescriptorBufferInfo* AllocatedBuffer::getBufferInfoPtr()
	{
		return &dbi;
	}

}

void oGFX::vkutils::tools::setImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkImageSubresourceRange subresourceRange, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
{
	{
		// Create an image barrier object
		VkImageMemoryBarrier imageMemoryBarrier = oGFX::vkutils::inits::imageMemoryBarrier();
		imageMemoryBarrier.oldLayout = oldImageLayout;
		imageMemoryBarrier.newLayout = newImageLayout;
		imageMemoryBarrier.image = image;
		imageMemoryBarrier.subresourceRange = subresourceRange;

		// Source layouts (old)
		// Source access mask controls actions that have to be finished on the old layout
		// before it will be transitioned to the new layout
		switch (oldImageLayout)
		{
		case VK_IMAGE_LAYOUT_UNDEFINED:
		// Image layout is undefined (or does not matter)
		// Only valid as initial layout
		// No flags required, listed only for completeness
		imageMemoryBarrier.srcAccessMask = 0;
		break;

		case VK_IMAGE_LAYOUT_PREINITIALIZED:
		// Image is preinitialized
		// Only valid as initial layout for linear images, preserves memory contents
		// Make sure host writes have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		// Image is a color attachment
		// Make sure any writes to the color buffer have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		// Image is a depth/stencil attachment
		// Make sure any writes to the depth/stencil buffer have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		// Image is a transfer source
		// Make sure any reads from the image have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		// Image is a transfer destination
		// Make sure any writes to the image have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		// Image is read by a shader
		// Make sure any shader reads from the image have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
		default:
		// Other source layouts aren't handled (yet)
		break;
		}

		// Target layouts (new)
		// Destination access mask controls the dependency for the new image layout
		switch (newImageLayout)
		{
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		// Image will be used as a transfer destination
		// Make sure any writes to the image have been finished
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		// Image will be used as a transfer source
		// Make sure any reads from the image have been finished
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		// Image will be used as a color attachment
		// Make sure any writes to the color buffer have been finished
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		// Image layout will be used as a depth/stencil attachment
		// Make sure any writes to depth/stencil buffer have been finished
		imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		// Image will be read in a shader (sampler, input attachment)
		// Make sure any writes to the image have been finished
		if (imageMemoryBarrier.srcAccessMask == 0)
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		}
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
		default:
		// Other source layouts aren't handled (yet)
		break;
		}

		// Put barrier inside setup command buffer
		vkCmdPipelineBarrier(
			cmdbuffer,
			srcStageMask,
			dstStageMask,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);
	}
}

void oGFX::vkutils::tools::setImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
{
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = aspectMask;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.layerCount = 1;
	setImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
}

void oGFX::vkutils::tools::insertImageMemoryBarrier(VkCommandBuffer cmdbuffer, VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkImageSubresourceRange subresourceRange)
{	
	VkImageMemoryBarrier imageMemoryBarrier = oGFX::vkutils::inits::imageMemoryBarrier();
	imageMemoryBarrier.srcAccessMask = srcAccessMask;
	imageMemoryBarrier.dstAccessMask = dstAccessMask;
	imageMemoryBarrier.oldLayout = oldImageLayout;
	imageMemoryBarrier.newLayout = newImageLayout;
	imageMemoryBarrier.image = image;
	imageMemoryBarrier.subresourceRange = subresourceRange;

	VkImageMemoryBarrier2 mb2{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
	mb2.dstAccessMask = dstAccessMask;
	mb2.srcAccessMask = srcAccessMask;
	mb2.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	mb2.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	mb2.oldLayout = oldImageLayout;
	mb2.newLayout = newImageLayout;
	mb2.image = image;
	mb2.subresourceRange = subresourceRange;
	VkDependencyInfo di{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
	di.pImageMemoryBarriers = &mb2;
	di.imageMemoryBarrierCount = 1;


	vkCmdPipelineBarrier2(cmdbuffer, &di);
	
	// vkCmdPipelineBarrier(
	// 	cmdbuffer,
	// 	srcStageMask,
	// 	dstStageMask,
	// 	0,
	// 	0, nullptr,
	// 	0, nullptr,
	// 	1, &imageMemoryBarrier);

}

size_t oGFX::vkutils::tools::UniformBufferPaddedSize(size_t size, size_t bufferMinAlignment)
{
	size_t result{ size };
	if (bufferMinAlignment > 0) {
		result = (size + bufferMinAlignment - 1) & ~(bufferMinAlignment - 1);
	}
	return result;
}

void oGFX::vkutils::tools::insertBufferMemoryBarrier(VkCommandBuffer cmdbuffer,uint32_t queueFamily
	, VkBuffer buffer
	, VkAccessFlags srcAccessMask
	, VkAccessFlags dstAccessMask
	, VkPipelineStageFlags srcStageMask
	, VkPipelineStageFlags dstStageMask
	, VkDeviceSize offset, VkDeviceSize range)
{
	VkBufferMemoryBarrier bmb{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
	bmb.buffer = buffer;
	bmb.srcAccessMask = srcAccessMask;
	bmb.dstAccessMask = dstAccessMask;
	bmb.offset = offset;
	bmb.size = range;
	bmb.srcQueueFamilyIndex = queueFamily;
	vkCmdPipelineBarrier(
		cmdbuffer,
		srcStageMask,
		dstStageMask,
		0,
		0, nullptr,
		1, &bmb,
		0, nullptr);
}

std::string oGFX::vkutils::tools::VkResultString(VkResult value)
{
	switch (value) {
	case (VK_SUCCESS): return "SUCCESS";
	case (VK_NOT_READY): return "NOT_READY";
	case (VK_TIMEOUT): return "TIMEOUT";
	case (VK_EVENT_SET): return "EVENT_SET";
	case (VK_EVENT_RESET): return "EVENT_RESET";
	case (VK_INCOMPLETE): return "INCOMPLETE";
	case (VK_ERROR_OUT_OF_HOST_MEMORY): return "ERROR_OUT_OF_HOST_MEMORY";
	case (VK_ERROR_OUT_OF_DEVICE_MEMORY): return "ERROR_OUT_OF_DEVICE_MEMORY";
	case (VK_ERROR_INITIALIZATION_FAILED): return "ERROR_INITIALIZATION_FAILED";
	case (VK_ERROR_DEVICE_LOST): return "ERROR_DEVICE_LOST";
	case (VK_ERROR_MEMORY_MAP_FAILED): return "ERROR_MEMORY_MAP_FAILED";
	case (VK_ERROR_LAYER_NOT_PRESENT): return "ERROR_LAYER_NOT_PRESENT";
	case (VK_ERROR_EXTENSION_NOT_PRESENT): return "ERROR_EXTENSION_NOT_PRESENT";
	case (VK_ERROR_FEATURE_NOT_PRESENT): return "ERROR_FEATURE_NOT_PRESENT";
	case (VK_ERROR_INCOMPATIBLE_DRIVER): return "ERROR_INCOMPATIBLE_DRIVER";
	case (VK_ERROR_TOO_MANY_OBJECTS): return "ERROR_TOO_MANY_OBJECTS";
	case (VK_ERROR_FORMAT_NOT_SUPPORTED): return "ERROR_FORMAT_NOT_SUPPORTED";
	case (VK_ERROR_FRAGMENTED_POOL): return "ERROR_FRAGMENTED_POOL";
	case (VK_ERROR_UNKNOWN): return "ERROR_UNKNOWN";
	case (VK_ERROR_OUT_OF_POOL_MEMORY): return "ERROR_OUT_OF_POOL_MEMORY";
	case (VK_ERROR_INVALID_EXTERNAL_HANDLE): return "ERROR_INVALID_EXTERNAL_HANDLE";
	case (VK_ERROR_FRAGMENTATION): return "ERROR_FRAGMENTATION";
	case (VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS): return "ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
	case (VK_PIPELINE_COMPILE_REQUIRED): return "PIPELINE_COMPILE_REQUIRED";
	case (VK_ERROR_SURFACE_LOST_KHR): return "ERROR_SURFACE_LOST_KHR";
	case (VK_ERROR_NATIVE_WINDOW_IN_USE_KHR): return "ERROR_NATIVE_WINDOW_IN_USE_KHR";
	case (VK_SUBOPTIMAL_KHR): return "SUBOPTIMAL_KHR";
	case (VK_ERROR_OUT_OF_DATE_KHR): return "ERROR_OUT_OF_DATE_KHR";
	case (VK_ERROR_INCOMPATIBLE_DISPLAY_KHR): return "ERROR_INCOMPATIBLE_DISPLAY_KHR";
	case (VK_ERROR_VALIDATION_FAILED_EXT): return "ERROR_VALIDATION_FAILED_EXT";
	case (VK_ERROR_INVALID_SHADER_NV): return "ERROR_INVALID_SHADER_NV";
	case (VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT): return "ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
	case (VK_ERROR_NOT_PERMITTED_KHR): return "ERROR_NOT_PERMITTED_KHR";
	case (VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT): return "ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
	case (VK_THREAD_IDLE_KHR): return "THREAD_IDLE_KHR";
	case (VK_THREAD_DONE_KHR): return "THREAD_DONE_KHR";
	case (VK_OPERATION_DEFERRED_KHR): return "OPERATION_DEFERRED_KHR";
	case (VK_OPERATION_NOT_DEFERRED_KHR): return "OPERATION_NOT_DEFERRED_KHR";
		//case (VK_ERROR_COMPRESSION_EXHAUSTED_EXT): return "ERROR_COMPRESSION_EXHAUSTED_EXT";
	default: return std::string("UNKNOWN_VkResult_value") + std::to_string(value);
	}
}

std::string oGFX::vkutils::tools::VkFormatString(VkFormat value) {
	switch (value) {
	case (VK_FORMAT_UNDEFINED): return "FORMAT_UNDEFINED";
	case (VK_FORMAT_R4G4_UNORM_PACK8): return "FORMAT_R4G4_UNORM_PACK8";
	case (VK_FORMAT_R4G4B4A4_UNORM_PACK16): return "FORMAT_R4G4B4A4_UNORM_PACK16";
	case (VK_FORMAT_B4G4R4A4_UNORM_PACK16): return "FORMAT_B4G4R4A4_UNORM_PACK16";
	case (VK_FORMAT_R5G6B5_UNORM_PACK16): return "FORMAT_R5G6B5_UNORM_PACK16";
	case (VK_FORMAT_B5G6R5_UNORM_PACK16): return "FORMAT_B5G6R5_UNORM_PACK16";
	case (VK_FORMAT_R5G5B5A1_UNORM_PACK16): return "FORMAT_R5G5B5A1_UNORM_PACK16";
	case (VK_FORMAT_B5G5R5A1_UNORM_PACK16): return "FORMAT_B5G5R5A1_UNORM_PACK16";
	case (VK_FORMAT_A1R5G5B5_UNORM_PACK16): return "FORMAT_A1R5G5B5_UNORM_PACK16";
	case (VK_FORMAT_R8_UNORM): return "FORMAT_R8_UNORM";
	case (VK_FORMAT_R8_SNORM): return "FORMAT_R8_SNORM";
	case (VK_FORMAT_R8_USCALED): return "FORMAT_R8_USCALED";
	case (VK_FORMAT_R8_SSCALED): return "FORMAT_R8_SSCALED";
	case (VK_FORMAT_R8_UINT): return "FORMAT_R8_UINT";
	case (VK_FORMAT_R8_SINT): return "FORMAT_R8_SINT";
	case (VK_FORMAT_R8_SRGB): return "FORMAT_R8_SRGB";
	case (VK_FORMAT_R8G8_UNORM): return "FORMAT_R8G8_UNORM";
	case (VK_FORMAT_R8G8_SNORM): return "FORMAT_R8G8_SNORM";
	case (VK_FORMAT_R8G8_USCALED): return "FORMAT_R8G8_USCALED";
	case (VK_FORMAT_R8G8_SSCALED): return "FORMAT_R8G8_SSCALED";
	case (VK_FORMAT_R8G8_UINT): return "FORMAT_R8G8_UINT";
	case (VK_FORMAT_R8G8_SINT): return "FORMAT_R8G8_SINT";
	case (VK_FORMAT_R8G8_SRGB): return "FORMAT_R8G8_SRGB";
	case (VK_FORMAT_R8G8B8_UNORM): return "FORMAT_R8G8B8_UNORM";
	case (VK_FORMAT_R8G8B8_SNORM): return "FORMAT_R8G8B8_SNORM";
	case (VK_FORMAT_R8G8B8_USCALED): return "FORMAT_R8G8B8_USCALED";
	case (VK_FORMAT_R8G8B8_SSCALED): return "FORMAT_R8G8B8_SSCALED";
	case (VK_FORMAT_R8G8B8_UINT): return "FORMAT_R8G8B8_UINT";
	case (VK_FORMAT_R8G8B8_SINT): return "FORMAT_R8G8B8_SINT";
	case (VK_FORMAT_R8G8B8_SRGB): return "FORMAT_R8G8B8_SRGB";
	case (VK_FORMAT_B8G8R8_UNORM): return "FORMAT_B8G8R8_UNORM";
	case (VK_FORMAT_B8G8R8_SNORM): return "FORMAT_B8G8R8_SNORM";
	case (VK_FORMAT_B8G8R8_USCALED): return "FORMAT_B8G8R8_USCALED";
	case (VK_FORMAT_B8G8R8_SSCALED): return "FORMAT_B8G8R8_SSCALED";
	case (VK_FORMAT_B8G8R8_UINT): return "FORMAT_B8G8R8_UINT";
	case (VK_FORMAT_B8G8R8_SINT): return "FORMAT_B8G8R8_SINT";
	case (VK_FORMAT_B8G8R8_SRGB): return "FORMAT_B8G8R8_SRGB";
	case (VK_FORMAT_R8G8B8A8_UNORM): return "FORMAT_R8G8B8A8_UNORM";
	case (VK_FORMAT_R8G8B8A8_SNORM): return "FORMAT_R8G8B8A8_SNORM";
	case (VK_FORMAT_R8G8B8A8_USCALED): return "FORMAT_R8G8B8A8_USCALED";
	case (VK_FORMAT_R8G8B8A8_SSCALED): return "FORMAT_R8G8B8A8_SSCALED";
	case (VK_FORMAT_R8G8B8A8_UINT): return "FORMAT_R8G8B8A8_UINT";
	case (VK_FORMAT_R8G8B8A8_SINT): return "FORMAT_R8G8B8A8_SINT";
	case (VK_FORMAT_R8G8B8A8_SRGB): return "FORMAT_R8G8B8A8_SRGB";
	case (VK_FORMAT_B8G8R8A8_UNORM): return "FORMAT_B8G8R8A8_UNORM";
	case (VK_FORMAT_B8G8R8A8_SNORM): return "FORMAT_B8G8R8A8_SNORM";
	case (VK_FORMAT_B8G8R8A8_USCALED): return "FORMAT_B8G8R8A8_USCALED";
	case (VK_FORMAT_B8G8R8A8_SSCALED): return "FORMAT_B8G8R8A8_SSCALED";
	case (VK_FORMAT_B8G8R8A8_UINT): return "FORMAT_B8G8R8A8_UINT";
	case (VK_FORMAT_B8G8R8A8_SINT): return "FORMAT_B8G8R8A8_SINT";
	case (VK_FORMAT_B8G8R8A8_SRGB): return "FORMAT_B8G8R8A8_SRGB";
	case (VK_FORMAT_A8B8G8R8_UNORM_PACK32): return "FORMAT_A8B8G8R8_UNORM_PACK32";
	case (VK_FORMAT_A8B8G8R8_SNORM_PACK32): return "FORMAT_A8B8G8R8_SNORM_PACK32";
	case (VK_FORMAT_A8B8G8R8_USCALED_PACK32): return "FORMAT_A8B8G8R8_USCALED_PACK32";
	case (VK_FORMAT_A8B8G8R8_SSCALED_PACK32): return "FORMAT_A8B8G8R8_SSCALED_PACK32";
	case (VK_FORMAT_A8B8G8R8_UINT_PACK32): return "FORMAT_A8B8G8R8_UINT_PACK32";
	case (VK_FORMAT_A8B8G8R8_SINT_PACK32): return "FORMAT_A8B8G8R8_SINT_PACK32";
	case (VK_FORMAT_A8B8G8R8_SRGB_PACK32): return "FORMAT_A8B8G8R8_SRGB_PACK32";
	case (VK_FORMAT_A2R10G10B10_UNORM_PACK32): return "FORMAT_A2R10G10B10_UNORM_PACK32";
	case (VK_FORMAT_A2R10G10B10_SNORM_PACK32): return "FORMAT_A2R10G10B10_SNORM_PACK32";
	case (VK_FORMAT_A2R10G10B10_USCALED_PACK32): return "FORMAT_A2R10G10B10_USCALED_PACK32";
	case (VK_FORMAT_A2R10G10B10_SSCALED_PACK32): return "FORMAT_A2R10G10B10_SSCALED_PACK32";
	case (VK_FORMAT_A2R10G10B10_UINT_PACK32): return "FORMAT_A2R10G10B10_UINT_PACK32";
	case (VK_FORMAT_A2R10G10B10_SINT_PACK32): return "FORMAT_A2R10G10B10_SINT_PACK32";
	case (VK_FORMAT_A2B10G10R10_UNORM_PACK32): return "FORMAT_A2B10G10R10_UNORM_PACK32";
	case (VK_FORMAT_A2B10G10R10_SNORM_PACK32): return "FORMAT_A2B10G10R10_SNORM_PACK32";
	case (VK_FORMAT_A2B10G10R10_USCALED_PACK32): return "FORMAT_A2B10G10R10_USCALED_PACK32";
	case (VK_FORMAT_A2B10G10R10_SSCALED_PACK32): return "FORMAT_A2B10G10R10_SSCALED_PACK32";
	case (VK_FORMAT_A2B10G10R10_UINT_PACK32): return "FORMAT_A2B10G10R10_UINT_PACK32";
	case (VK_FORMAT_A2B10G10R10_SINT_PACK32): return "FORMAT_A2B10G10R10_SINT_PACK32";
	case (VK_FORMAT_R16_UNORM): return "FORMAT_R16_UNORM";
	case (VK_FORMAT_R16_SNORM): return "FORMAT_R16_SNORM";
	case (VK_FORMAT_R16_USCALED): return "FORMAT_R16_USCALED";
	case (VK_FORMAT_R16_SSCALED): return "FORMAT_R16_SSCALED";
	case (VK_FORMAT_R16_UINT): return "FORMAT_R16_UINT";
	case (VK_FORMAT_R16_SINT): return "FORMAT_R16_SINT";
	case (VK_FORMAT_R16_SFLOAT): return "FORMAT_R16_SFLOAT";
	case (VK_FORMAT_R16G16_UNORM): return "FORMAT_R16G16_UNORM";
	case (VK_FORMAT_R16G16_SNORM): return "FORMAT_R16G16_SNORM";
	case (VK_FORMAT_R16G16_USCALED): return "FORMAT_R16G16_USCALED";
	case (VK_FORMAT_R16G16_SSCALED): return "FORMAT_R16G16_SSCALED";
	case (VK_FORMAT_R16G16_UINT): return "FORMAT_R16G16_UINT";
	case (VK_FORMAT_R16G16_SINT): return "FORMAT_R16G16_SINT";
	case (VK_FORMAT_R16G16_SFLOAT): return "FORMAT_R16G16_SFLOAT";
	case (VK_FORMAT_R16G16B16_UNORM): return "FORMAT_R16G16B16_UNORM";
	case (VK_FORMAT_R16G16B16_SNORM): return "FORMAT_R16G16B16_SNORM";
	case (VK_FORMAT_R16G16B16_USCALED): return "FORMAT_R16G16B16_USCALED";
	case (VK_FORMAT_R16G16B16_SSCALED): return "FORMAT_R16G16B16_SSCALED";
	case (VK_FORMAT_R16G16B16_UINT): return "FORMAT_R16G16B16_UINT";
	case (VK_FORMAT_R16G16B16_SINT): return "FORMAT_R16G16B16_SINT";
	case (VK_FORMAT_R16G16B16_SFLOAT): return "FORMAT_R16G16B16_SFLOAT";
	case (VK_FORMAT_R16G16B16A16_UNORM): return "FORMAT_R16G16B16A16_UNORM";
	case (VK_FORMAT_R16G16B16A16_SNORM): return "FORMAT_R16G16B16A16_SNORM";
	case (VK_FORMAT_R16G16B16A16_USCALED): return "FORMAT_R16G16B16A16_USCALED";
	case (VK_FORMAT_R16G16B16A16_SSCALED): return "FORMAT_R16G16B16A16_SSCALED";
	case (VK_FORMAT_R16G16B16A16_UINT): return "FORMAT_R16G16B16A16_UINT";
	case (VK_FORMAT_R16G16B16A16_SINT): return "FORMAT_R16G16B16A16_SINT";
	case (VK_FORMAT_R16G16B16A16_SFLOAT): return "FORMAT_R16G16B16A16_SFLOAT";
	case (VK_FORMAT_R32_UINT): return "FORMAT_R32_UINT";
	case (VK_FORMAT_R32_SINT): return "FORMAT_R32_SINT";
	case (VK_FORMAT_R32_SFLOAT): return "FORMAT_R32_SFLOAT";
	case (VK_FORMAT_R32G32_UINT): return "FORMAT_R32G32_UINT";
	case (VK_FORMAT_R32G32_SINT): return "FORMAT_R32G32_SINT";
	case (VK_FORMAT_R32G32_SFLOAT): return "FORMAT_R32G32_SFLOAT";
	case (VK_FORMAT_R32G32B32_UINT): return "FORMAT_R32G32B32_UINT";
	case (VK_FORMAT_R32G32B32_SINT): return "FORMAT_R32G32B32_SINT";
	case (VK_FORMAT_R32G32B32_SFLOAT): return "FORMAT_R32G32B32_SFLOAT";
	case (VK_FORMAT_R32G32B32A32_UINT): return "FORMAT_R32G32B32A32_UINT";
	case (VK_FORMAT_R32G32B32A32_SINT): return "FORMAT_R32G32B32A32_SINT";
	case (VK_FORMAT_R32G32B32A32_SFLOAT): return "FORMAT_R32G32B32A32_SFLOAT";
	case (VK_FORMAT_R64_UINT): return "FORMAT_R64_UINT";
	case (VK_FORMAT_R64_SINT): return "FORMAT_R64_SINT";
	case (VK_FORMAT_R64_SFLOAT): return "FORMAT_R64_SFLOAT";
	case (VK_FORMAT_R64G64_UINT): return "FORMAT_R64G64_UINT";
	case (VK_FORMAT_R64G64_SINT): return "FORMAT_R64G64_SINT";
	case (VK_FORMAT_R64G64_SFLOAT): return "FORMAT_R64G64_SFLOAT";
	case (VK_FORMAT_R64G64B64_UINT): return "FORMAT_R64G64B64_UINT";
	case (VK_FORMAT_R64G64B64_SINT): return "FORMAT_R64G64B64_SINT";
	case (VK_FORMAT_R64G64B64_SFLOAT): return "FORMAT_R64G64B64_SFLOAT";
	case (VK_FORMAT_R64G64B64A64_UINT): return "FORMAT_R64G64B64A64_UINT";
	case (VK_FORMAT_R64G64B64A64_SINT): return "FORMAT_R64G64B64A64_SINT";
	case (VK_FORMAT_R64G64B64A64_SFLOAT): return "FORMAT_R64G64B64A64_SFLOAT";
	case (VK_FORMAT_B10G11R11_UFLOAT_PACK32): return "FORMAT_B10G11R11_UFLOAT_PACK32";
	case (VK_FORMAT_E5B9G9R9_UFLOAT_PACK32): return "FORMAT_E5B9G9R9_UFLOAT_PACK32";
	case (VK_FORMAT_D16_UNORM): return "FORMAT_D16_UNORM";
	case (VK_FORMAT_X8_D24_UNORM_PACK32): return "FORMAT_X8_D24_UNORM_PACK32";
	case (VK_FORMAT_D32_SFLOAT): return "FORMAT_D32_SFLOAT";
	case (VK_FORMAT_S8_UINT): return "FORMAT_S8_UINT";
	case (VK_FORMAT_D16_UNORM_S8_UINT): return "FORMAT_D16_UNORM_S8_UINT";
	case (VK_FORMAT_D24_UNORM_S8_UINT): return "FORMAT_D24_UNORM_S8_UINT";
	case (VK_FORMAT_D32_SFLOAT_S8_UINT): return "FORMAT_D32_SFLOAT_S8_UINT";
	case (VK_FORMAT_BC1_RGB_UNORM_BLOCK): return "FORMAT_BC1_RGB_UNORM_BLOCK";
	case (VK_FORMAT_BC1_RGB_SRGB_BLOCK): return "FORMAT_BC1_RGB_SRGB_BLOCK";
	case (VK_FORMAT_BC1_RGBA_UNORM_BLOCK): return "FORMAT_BC1_RGBA_UNORM_BLOCK";
	case (VK_FORMAT_BC1_RGBA_SRGB_BLOCK): return "FORMAT_BC1_RGBA_SRGB_BLOCK";
	case (VK_FORMAT_BC2_UNORM_BLOCK): return "FORMAT_BC2_UNORM_BLOCK";
	case (VK_FORMAT_BC2_SRGB_BLOCK): return "FORMAT_BC2_SRGB_BLOCK";
	case (VK_FORMAT_BC3_UNORM_BLOCK): return "FORMAT_BC3_UNORM_BLOCK";
	case (VK_FORMAT_BC3_SRGB_BLOCK): return "FORMAT_BC3_SRGB_BLOCK";
	case (VK_FORMAT_BC4_UNORM_BLOCK): return "FORMAT_BC4_UNORM_BLOCK";
	case (VK_FORMAT_BC4_SNORM_BLOCK): return "FORMAT_BC4_SNORM_BLOCK";
	case (VK_FORMAT_BC5_UNORM_BLOCK): return "FORMAT_BC5_UNORM_BLOCK";
	case (VK_FORMAT_BC5_SNORM_BLOCK): return "FORMAT_BC5_SNORM_BLOCK";
	case (VK_FORMAT_BC6H_UFLOAT_BLOCK): return "FORMAT_BC6H_UFLOAT_BLOCK";
	case (VK_FORMAT_BC6H_SFLOAT_BLOCK): return "FORMAT_BC6H_SFLOAT_BLOCK";
	case (VK_FORMAT_BC7_UNORM_BLOCK): return "FORMAT_BC7_UNORM_BLOCK";
	case (VK_FORMAT_BC7_SRGB_BLOCK): return "FORMAT_BC7_SRGB_BLOCK";
	case (VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK): return "FORMAT_ETC2_R8G8B8_UNORM_BLOCK";
	case (VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK): return "FORMAT_ETC2_R8G8B8_SRGB_BLOCK";
	case (VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK): return "FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK";
	case (VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK): return "FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK";
	case (VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK): return "FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK";
	case (VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK): return "FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK";
	case (VK_FORMAT_EAC_R11_UNORM_BLOCK): return "FORMAT_EAC_R11_UNORM_BLOCK";
	case (VK_FORMAT_EAC_R11_SNORM_BLOCK): return "FORMAT_EAC_R11_SNORM_BLOCK";
	case (VK_FORMAT_EAC_R11G11_UNORM_BLOCK): return "FORMAT_EAC_R11G11_UNORM_BLOCK";
	case (VK_FORMAT_EAC_R11G11_SNORM_BLOCK): return "FORMAT_EAC_R11G11_SNORM_BLOCK";
	case (VK_FORMAT_ASTC_4x4_UNORM_BLOCK): return "FORMAT_ASTC_4x4_UNORM_BLOCK";
	case (VK_FORMAT_ASTC_4x4_SRGB_BLOCK): return "FORMAT_ASTC_4x4_SRGB_BLOCK";
	case (VK_FORMAT_ASTC_5x4_UNORM_BLOCK): return "FORMAT_ASTC_5x4_UNORM_BLOCK";
	case (VK_FORMAT_ASTC_5x4_SRGB_BLOCK): return "FORMAT_ASTC_5x4_SRGB_BLOCK";
	case (VK_FORMAT_ASTC_5x5_UNORM_BLOCK): return "FORMAT_ASTC_5x5_UNORM_BLOCK";
	case (VK_FORMAT_ASTC_5x5_SRGB_BLOCK): return "FORMAT_ASTC_5x5_SRGB_BLOCK";
	case (VK_FORMAT_ASTC_6x5_UNORM_BLOCK): return "FORMAT_ASTC_6x5_UNORM_BLOCK";
	case (VK_FORMAT_ASTC_6x5_SRGB_BLOCK): return "FORMAT_ASTC_6x5_SRGB_BLOCK";
	case (VK_FORMAT_ASTC_6x6_UNORM_BLOCK): return "FORMAT_ASTC_6x6_UNORM_BLOCK";
	case (VK_FORMAT_ASTC_6x6_SRGB_BLOCK): return "FORMAT_ASTC_6x6_SRGB_BLOCK";
	case (VK_FORMAT_ASTC_8x5_UNORM_BLOCK): return "FORMAT_ASTC_8x5_UNORM_BLOCK";
	case (VK_FORMAT_ASTC_8x5_SRGB_BLOCK): return "FORMAT_ASTC_8x5_SRGB_BLOCK";
	case (VK_FORMAT_ASTC_8x6_UNORM_BLOCK): return "FORMAT_ASTC_8x6_UNORM_BLOCK";
	case (VK_FORMAT_ASTC_8x6_SRGB_BLOCK): return "FORMAT_ASTC_8x6_SRGB_BLOCK";
	case (VK_FORMAT_ASTC_8x8_UNORM_BLOCK): return "FORMAT_ASTC_8x8_UNORM_BLOCK";
	case (VK_FORMAT_ASTC_8x8_SRGB_BLOCK): return "FORMAT_ASTC_8x8_SRGB_BLOCK";
	case (VK_FORMAT_ASTC_10x5_UNORM_BLOCK): return "FORMAT_ASTC_10x5_UNORM_BLOCK";
	case (VK_FORMAT_ASTC_10x5_SRGB_BLOCK): return "FORMAT_ASTC_10x5_SRGB_BLOCK";
	case (VK_FORMAT_ASTC_10x6_UNORM_BLOCK): return "FORMAT_ASTC_10x6_UNORM_BLOCK";
	case (VK_FORMAT_ASTC_10x6_SRGB_BLOCK): return "FORMAT_ASTC_10x6_SRGB_BLOCK";
	case (VK_FORMAT_ASTC_10x8_UNORM_BLOCK): return "FORMAT_ASTC_10x8_UNORM_BLOCK";
	case (VK_FORMAT_ASTC_10x8_SRGB_BLOCK): return "FORMAT_ASTC_10x8_SRGB_BLOCK";
	case (VK_FORMAT_ASTC_10x10_UNORM_BLOCK): return "FORMAT_ASTC_10x10_UNORM_BLOCK";
	case (VK_FORMAT_ASTC_10x10_SRGB_BLOCK): return "FORMAT_ASTC_10x10_SRGB_BLOCK";
	case (VK_FORMAT_ASTC_12x10_UNORM_BLOCK): return "FORMAT_ASTC_12x10_UNORM_BLOCK";
	case (VK_FORMAT_ASTC_12x10_SRGB_BLOCK): return "FORMAT_ASTC_12x10_SRGB_BLOCK";
	case (VK_FORMAT_ASTC_12x12_UNORM_BLOCK): return "FORMAT_ASTC_12x12_UNORM_BLOCK";
	case (VK_FORMAT_ASTC_12x12_SRGB_BLOCK): return "FORMAT_ASTC_12x12_SRGB_BLOCK";
	case (VK_FORMAT_G8B8G8R8_422_UNORM): return "FORMAT_G8B8G8R8_422_UNORM";
	case (VK_FORMAT_B8G8R8G8_422_UNORM): return "FORMAT_B8G8R8G8_422_UNORM";
	case (VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM): return "FORMAT_G8_B8_R8_3PLANE_420_UNORM";
	case (VK_FORMAT_G8_B8R8_2PLANE_420_UNORM): return "FORMAT_G8_B8R8_2PLANE_420_UNORM";
	case (VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM): return "FORMAT_G8_B8_R8_3PLANE_422_UNORM";
	case (VK_FORMAT_G8_B8R8_2PLANE_422_UNORM): return "FORMAT_G8_B8R8_2PLANE_422_UNORM";
	case (VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM): return "FORMAT_G8_B8_R8_3PLANE_444_UNORM";
	case (VK_FORMAT_R10X6_UNORM_PACK16): return "FORMAT_R10X6_UNORM_PACK16";
	case (VK_FORMAT_R10X6G10X6_UNORM_2PACK16): return "FORMAT_R10X6G10X6_UNORM_2PACK16";
	case (VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16): return "FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16";
	case (VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16): return "FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16";
	case (VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16): return "FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16";
	case (VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16): return "FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16";
	case (VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16): return "FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16";
	case (VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16): return "FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16";
	case (VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16): return "FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16";
	case (VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16): return "FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16";
	case (VK_FORMAT_R12X4_UNORM_PACK16): return "FORMAT_R12X4_UNORM_PACK16";
	case (VK_FORMAT_R12X4G12X4_UNORM_2PACK16): return "FORMAT_R12X4G12X4_UNORM_2PACK16";
	case (VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16): return "FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16";
	case (VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16): return "FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16";
	case (VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16): return "FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16";
	case (VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16): return "FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16";
	case (VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16): return "FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16";
	case (VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16): return "FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16";
	case (VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16): return "FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16";
	case (VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16): return "FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16";
	case (VK_FORMAT_G16B16G16R16_422_UNORM): return "FORMAT_G16B16G16R16_422_UNORM";
	case (VK_FORMAT_B16G16R16G16_422_UNORM): return "FORMAT_B16G16R16G16_422_UNORM";
	case (VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM): return "FORMAT_G16_B16_R16_3PLANE_420_UNORM";
	case (VK_FORMAT_G16_B16R16_2PLANE_420_UNORM): return "FORMAT_G16_B16R16_2PLANE_420_UNORM";
	case (VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM): return "FORMAT_G16_B16_R16_3PLANE_422_UNORM";
	case (VK_FORMAT_G16_B16R16_2PLANE_422_UNORM): return "FORMAT_G16_B16R16_2PLANE_422_UNORM";
	case (VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM): return "FORMAT_G16_B16_R16_3PLANE_444_UNORM";
	case (VK_FORMAT_G8_B8R8_2PLANE_444_UNORM): return "FORMAT_G8_B8R8_2PLANE_444_UNORM";
	case (VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16): return "FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16";
	case (VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16): return "FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16";
	case (VK_FORMAT_G16_B16R16_2PLANE_444_UNORM): return "FORMAT_G16_B16R16_2PLANE_444_UNORM";
	case (VK_FORMAT_A4R4G4B4_UNORM_PACK16): return "FORMAT_A4R4G4B4_UNORM_PACK16";
	case (VK_FORMAT_A4B4G4R4_UNORM_PACK16): return "FORMAT_A4B4G4R4_UNORM_PACK16";
	case (VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK): return "FORMAT_ASTC_4x4_SFLOAT_BLOCK";
	case (VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK): return "FORMAT_ASTC_5x4_SFLOAT_BLOCK";
	case (VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK): return "FORMAT_ASTC_5x5_SFLOAT_BLOCK";
	case (VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK): return "FORMAT_ASTC_6x5_SFLOAT_BLOCK";
	case (VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK): return "FORMAT_ASTC_6x6_SFLOAT_BLOCK";
	case (VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK): return "FORMAT_ASTC_8x5_SFLOAT_BLOCK";
	case (VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK): return "FORMAT_ASTC_8x6_SFLOAT_BLOCK";
	case (VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK): return "FORMAT_ASTC_8x8_SFLOAT_BLOCK";
	case (VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK): return "FORMAT_ASTC_10x5_SFLOAT_BLOCK";
	case (VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK): return "FORMAT_ASTC_10x6_SFLOAT_BLOCK";
	case (VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK): return "FORMAT_ASTC_10x8_SFLOAT_BLOCK";
	case (VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK): return "FORMAT_ASTC_10x10_SFLOAT_BLOCK";
	case (VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK): return "FORMAT_ASTC_12x10_SFLOAT_BLOCK";
	case (VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK): return "FORMAT_ASTC_12x12_SFLOAT_BLOCK";
	case (VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG): return "FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG";
	case (VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG): return "FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG";
	case (VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG): return "FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG";
	case (VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG): return "FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG";
	case (VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG): return "FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG";
	case (VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG): return "FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG";
	case (VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG): return "FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG";
	case (VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG): return "FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG";
	default: return std::string("UNKNOWN_VkFormat_value") + std::to_string(value);
	}
}

std::string oGFX::vkutils::tools::VkColorSpaceKHRString(VkColorSpaceKHR value) {
	switch (value) {
	case (VK_COLOR_SPACE_SRGB_NONLINEAR_KHR): return "COLOR_SPACE_SRGB_NONLINEAR_KHR";
	case (VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT): return "COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT";
	case (VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT): return "COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT";
	case (VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT): return "COLOR_SPACE_DISPLAY_P3_LINEAR_EXT";
	case (VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT): return "COLOR_SPACE_DCI_P3_NONLINEAR_EXT";
	case (VK_COLOR_SPACE_BT709_LINEAR_EXT): return "COLOR_SPACE_BT709_LINEAR_EXT";
	case (VK_COLOR_SPACE_BT709_NONLINEAR_EXT): return "COLOR_SPACE_BT709_NONLINEAR_EXT";
	case (VK_COLOR_SPACE_BT2020_LINEAR_EXT): return "COLOR_SPACE_BT2020_LINEAR_EXT";
	case (VK_COLOR_SPACE_HDR10_ST2084_EXT): return "COLOR_SPACE_HDR10_ST2084_EXT";
	case (VK_COLOR_SPACE_DOLBYVISION_EXT): return "COLOR_SPACE_DOLBYVISION_EXT";
	case (VK_COLOR_SPACE_HDR10_HLG_EXT): return "COLOR_SPACE_HDR10_HLG_EXT";
	case (VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT): return "COLOR_SPACE_ADOBERGB_LINEAR_EXT";
	case (VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT): return "COLOR_SPACE_ADOBERGB_NONLINEAR_EXT";
	case (VK_COLOR_SPACE_PASS_THROUGH_EXT): return "COLOR_SPACE_PASS_THROUGH_EXT";
	case (VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT): return "COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT";
	case (VK_COLOR_SPACE_DISPLAY_NATIVE_AMD): return "COLOR_SPACE_DISPLAY_NATIVE_AMD";
	default: return std::string("UNKNOWN_VkColorSpaceKHR_value ") + std::to_string(value);
	}
}

std::string oGFX::vkutils::tools::VkImageLayoutString(VkImageLayout value)
{
	switch (value)
	{
	case(VK_IMAGE_LAYOUT_UNDEFINED):return "VK_IMAGE_LAYOUT_UNDEFINED";
	case(VK_IMAGE_LAYOUT_GENERAL):return "VK_IMAGE_LAYOUT_GENERAL";
	case(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL):return "VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL";
	case(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL):return "VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL";
	case(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL):return "VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL";
	case(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL):return "VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL";
	case(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL):return "VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL";
	case(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL):return "VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL";
	case(VK_IMAGE_LAYOUT_PREINITIALIZED):return "VK_IMAGE_LAYOUT_PREINITIALIZED";
	case(VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL):return "VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL";
	case(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL):return "VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL";
	case(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL):return "VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL";
	case(VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL):return "VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL";
	case(VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL):return "VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL";
	case(VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL):return "VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL";
	case(VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL):return "VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL";
	case(VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL):return "VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL";
	case(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR):return "VK_IMAGE_LAYOUT_PRESENT_SRC_KHR";	
	case(VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR):return "VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR";
	case(VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT):return "VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT";
	case(VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR):return "VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR";
#ifdef VK_ENABLE_BETA_EXTENSIONS
	case(VK_IMAGE_LAYOUT_VIDEO_ENCODE_DST_KHR):return "VK_IMAGE_LAYOUT_VIDEO_ENCODE_DST_KHR";
	case(VK_IMAGE_LAYOUT_VIDEO_ENCODE_SRC_KHR):return "VK_IMAGE_LAYOUT_VIDEO_ENCODE_SRC_KHR";
	case(VK_IMAGE_LAYOUT_VIDEO_ENCODE_DPB_KHR):return "VK_IMAGE_LAYOUT_VIDEO_ENCODE_DPB_KHR";
	case(VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR):return "VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR";
	case(VK_IMAGE_LAYOUT_VIDEO_DECODE_SRC_KHR):return "VK_IMAGE_LAYOUT_VIDEO_DECODE_SRC_KHR";
	case(VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR):return "VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR";
#endif
	//case(VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL_KHR):return "VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL,";
	//case(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL_KHR):return "VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL,";
	//case(VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV):return "VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR,";
	//case(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR):return "VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,";
	//case(VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL_KHR):return "VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL,";
	//case(VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL_KHR):return "VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL,";
	//case(VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL_KHR):return "VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL,";
	//case(VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR):return "VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,";
	//case(VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR):return "VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,";
	case(VK_IMAGE_LAYOUT_MAX_ENUM):return "VK_IMAGE_LAYOUT_MAX_ENUM";
	default: return std::string("UNKNOWN_VkImageLayout_value ") + std::to_string(value);
	}
}

VkDebugReportObjectTypeEXT GetDebugNameExtTypeByID(std::type_index id)
{
	static std::unordered_map<std::type_index, VkDebugReportObjectTypeEXT> mapDebugNameExtTypeByID
	{
		{ std::type_index(typeid(VkInstance)),VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT },
		{ std::type_index(typeid(VkPhysicalDevice)),VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT },
		{ std::type_index(typeid(VkDevice)),VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT },
		{ std::type_index(typeid(VkQueue)),VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT },
		{ std::type_index(typeid(VkSemaphore)),VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT },
		{ std::type_index(typeid(VkCommandBuffer)),VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT },
		{ std::type_index(typeid(VkFence)),VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT },
		{ std::type_index(typeid(VkFence)),VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT },
		{ std::type_index(typeid(VkDeviceMemory)), VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT},
		{ std::type_index(typeid(VkBuffer)), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT},
		{ std::type_index(typeid(VkImage)), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT},
		{ std::type_index(typeid(VkEvent)), VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT},
		{ std::type_index(typeid(VkQueryPool)), VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT},
		{ std::type_index(typeid(VkBufferView)), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT},
		{ std::type_index(typeid(VkImageView)), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT},
		{ std::type_index(typeid(VkShaderModule)), VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT},
		{ std::type_index(typeid(VkPipelineCache)), VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT},
		{ std::type_index(typeid(VkPipelineLayout)), VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT},
		{ std::type_index(typeid(VkRenderPass)), VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT},
		{ std::type_index(typeid(VkPipeline)), VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT},
		{ std::type_index(typeid(VkDescriptorSetLayout)), VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT},
		{ std::type_index(typeid(VkSampler)), VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT},
		{ std::type_index(typeid(VkDescriptorPool)), VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT},
		{ std::type_index(typeid(VkDescriptorSet)), VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT},
		{ std::type_index(typeid(VkFramebuffer)), VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT},
		{ std::type_index(typeid(VkCommandPool)), VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT},
		{ std::type_index(typeid(VkSurfaceKHR)), VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT},
		{ std::type_index(typeid(VkSwapchainKHR)), VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT},
		{ std::type_index(typeid(VkDebugReportCallbackEXT)), VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT_EXT},
		{ std::type_index(typeid(VkDisplayKHR)), VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_KHR_EXT},
		{ std::type_index(typeid(VkDisplayModeKHR)), VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_MODE_KHR_EXT},
		{ std::type_index(typeid(VkValidationCacheEXT)), VK_DEBUG_REPORT_OBJECT_TYPE_VALIDATION_CACHE_EXT_EXT},
		{ std::type_index(typeid(VkSamplerYcbcrConversion)), VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION_EXT},
		{ std::type_index(typeid(VkDescriptorUpdateTemplate)), VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_EXT},
		{ std::type_index(typeid(VkCuModuleNVX)), VK_DEBUG_REPORT_OBJECT_TYPE_CU_MODULE_NVX_EXT},
		{ std::type_index(typeid(VkCuFunctionNVX)), VK_DEBUG_REPORT_OBJECT_TYPE_CU_FUNCTION_NVX_EXT},
		{ std::type_index(typeid(VkAccelerationStructureKHR)), VK_DEBUG_REPORT_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR_EXT},
		{ std::type_index(typeid(VkAccelerationStructureNV)), VK_DEBUG_REPORT_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV_EXT},
		{ std::type_index(typeid(VK_OBJECT_TYPE_BUFFER_COLLECTION_FUCHSIA)), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_COLLECTION_FUCHSIA_EXT},
		{ std::type_index(typeid(VkDebugReportCallbackEXT)), VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT},
		{ std::type_index(typeid(VkValidationCacheEXT)), VK_DEBUG_REPORT_OBJECT_TYPE_VALIDATION_CACHE_EXT},
		{ std::type_index(typeid(VkDescriptorUpdateTemplateKHR)), VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_KHR_EXT},
		{ std::type_index(typeid(VkSamplerYcbcrConversionKHR)), VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION_KHR_EXT},
	};

	auto ret = mapDebugNameExtTypeByID[id];
	if (ret == 0) return VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT;	

	return ret;	
}

GPUTransform ConstructGPUTransform(const glm::mat4& m, const glm::mat4& inv, const glm::mat4& p)
{
	GPUTransform g;

	g.row0 = vec4(m[0][0], m[1][0], m[2][0], m[3][0]);
	g.row1 = vec4(m[0][1], m[1][1], m[2][1], m[3][1]);
	g.row2 = vec4(m[0][2], m[1][2], m[2][2], m[3][2]);

	g.invRow0 = vec4(inv[0][0], inv[1][0], inv[2][0], inv[3][0]);
	g.invRow1 = vec4(inv[0][1], inv[1][1], inv[2][1], inv[3][1]);
	g.invRow2 = vec4(inv[0][2], inv[1][2], inv[2][2], inv[3][2]);

	g.prevRow0 = vec4(p[0][0], p[1][0], p[2][0], p[3][0]);
	g.prevRow1 = vec4(p[0][1], p[1][1], p[2][1], p[3][1]);
	g.prevRow2 = vec4(p[0][2], p[1][2], p[2][2], p[3][2]);

	return g;
}

void ffxSpdSetup(uint32_t* dispatchThreadGroupCountXY, uint32_t* workGroupOffset, uint32_t* numWorkGroupsAndMips, uint32_t* rectInfo, int32_t mips)
{
	// determines the offset of the first tile to downsample based on
	// left (rectInfo[0]) and top (rectInfo[1]) of the subregion.
	workGroupOffset[0] = rectInfo[0] / 64;
	workGroupOffset[1] = rectInfo[1] / 64;

	uint32_t endIndexX = (rectInfo[0] + rectInfo[2] - 1) / 64;  // rectInfo[0] = left, rectInfo[2] = width
	uint32_t endIndexY = (rectInfo[1] + rectInfo[3] - 1) / 64;  // rectInfo[1] = top, rectInfo[3] = height

	// we only need to dispatch as many thread groups as tiles we need to downsample
	// number of tiles per slice depends on the subregion to downsample
	dispatchThreadGroupCountXY[0] = endIndexX + 1 - workGroupOffset[0];
	dispatchThreadGroupCountXY[1] = endIndexY + 1 - workGroupOffset[1];

	// number of thread groups per slice
	numWorkGroupsAndMips[0] = (dispatchThreadGroupCountXY[0]) * (dispatchThreadGroupCountXY[1]);

	if (mips >= 0)
	{
		numWorkGroupsAndMips[1] = uint32_t(mips);
	}
	else
	{
		// calculate based on rect width and height
		uint32_t resolution = std::max(rectInfo[2], rectInfo[3]);
		numWorkGroupsAndMips[1] = uint32_t((std::min(std::floor(std::log2(float(resolution))), float(12))));
	}
}

