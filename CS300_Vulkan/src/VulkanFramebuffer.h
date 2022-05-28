#pragma once
#include <cstdint>
#include <vulkan/vulkan.h>
#include "VulkanDevice.h"

//Framebuffer for offscreen rendering
struct VulkanFramebuffer
{
	VkImage image;
	VkDeviceMemory mem;
	VkImageView view;
	VkFormat format;
	VkImageLayout layout;

	void createAttachment(VulkanDevice& indevice, uint32_t width, uint32_t height,
		VkFormat format, VkImageUsageFlagBits usage);

	void destroy(VkDevice device);
};
struct FrameBuffer
{
	int32_t width, height;
	VkFramebuffer frameBuffer;
	VulkanFramebuffer position, normal, albedo;
	VulkanFramebuffer depth;
	VkRenderPass renderPass;
};