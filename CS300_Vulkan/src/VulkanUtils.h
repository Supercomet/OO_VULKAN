#pragma once
#include <vulkan/vulkan.h>
#include "glm/glm.hpp"

#include <string>
#include <vector>
struct VulkanInstance;
struct VulkanDevice;
namespace oGFX
{

	glm::mat4 customOrtho(float aspect_ratio, float size, float nr, float fr);
	
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

	struct Vertex
	{
		//float pos[3] ; // Vertex position (x, y, z)
		//float col[3] ; // Vertex colour (r, g, b)
		//float tex[2] ; // Texture Coords(u,v)
		glm::vec3 pos; // Vertex position (x, y, z)
		glm::vec3 col; // Vertex colour (r, g, b)
		glm::vec2 tex; // Texture Coords(u,v)
	};

	oGFX::SwapChainDetails GetSwapchainDetails(VulkanInstance& instance,VkPhysicalDevice device);
	oGFX::QueueFamilyIndices GetQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

	VkSurfaceFormatKHR ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR ChooseBestPresentationMode(const std::vector<VkPresentModeKHR> presentationModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);

	VkImageView CreateImageView(VulkanDevice& device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

	VkFormat ChooseSupportedFormat(VulkanDevice& device, const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);

	VkImage CreateImage(VulkanDevice& device,uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory* imageMemory);

	uint32_t FindMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes, VkMemoryPropertyFlags properties);

	VkShaderModule CreateShaderModule(VulkanDevice& device, const std::vector<char>& code);

	std::vector<char> readFile(const std::string& filename);

	void CreateBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize bufferSize,
		VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags bufferProperties,
		VkBuffer* buffer, VkDeviceMemory* bufferMemory);

	void CopyBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool,
		VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize);

	VkCommandBuffer beginCommandBuffer(VkDevice device, VkCommandPool commandPool);

	void endAndSubmitCommandBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer);
	
	void TransitionImageLayout(VkDevice device, VkQueue queue, VkCommandPool commandPool,
		VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

	void CopyImageBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool,
		VkBuffer srcBuffer, VkImage image, uint32_t width, uint32_t height);

	unsigned char* LoadTextureFromFile(const std::string& fileName, int& width, int& height, uint64_t& imageSize);
}

