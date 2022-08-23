#include "VulkanSwapchain.h"
#include  <stdexcept>
#include  <array>


#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanUtils.h"

VulkanSwapchain::~VulkanSwapchain()
{
	depthAttachment.destroy(m_devicePtr->logicalDevice);
	for (auto& img : swapChainImages)
	{
		vkDestroyImageView(m_devicePtr->logicalDevice, img.imageView, nullptr);
	}
	if (swapchain)
	{
		vkDestroySwapchainKHR(m_devicePtr->logicalDevice,swapchain, nullptr);
	}
}

void VulkanSwapchain::Init(VulkanInstance& instance, VulkanDevice& device)
{
	using namespace oGFX;
	m_devicePtr = &device;

	//wait until nothing is using it
	vkDeviceWaitIdle(device.logicalDevice);

	VkSwapchainKHR oldSwapchain = swapchain;
	if (oldSwapchain)
	{
		for (auto& img : swapChainImages)
		{
			vkDestroyImageView(m_devicePtr->logicalDevice, img.imageView, nullptr);
		}
		swapChainImages.clear();
	}

	// get swap chain details so we can pick best settings
	SwapChainDetails swapChainDetails = oGFX::GetSwapchainDetails(instance,device.physicalDevice);
	minImageCount = swapChainDetails.surfaceCapabilities.minImageCount;

	//find optimal surface values for our swapchain
	//1 choose best surface format
	VkSurfaceFormatKHR surfaceFormat = ChooseBestSurfaceFormat(swapChainDetails.formats);
	//2 choose best presentation mode
	VkPresentModeKHR presentMode = ChooseBestPresentationMode(swapChainDetails.presentationModes);
	//3 choose swapchain image size / resolution
	VkExtent2D extent = ChooseSwapExtent(swapChainDetails.surfaceCapabilities);

	//how many images in our swapchain
	//get one more than the minimum for triple buffering
	uint32_t imageCount = swapChainDetails.surfaceCapabilities.minImageCount + 1;

	//if image count is higher than max clamp down to max.
	//if maximagecount is 0, we are unrestricted 
	if (swapChainDetails.surfaceCapabilities.maxImageCount > 0 && swapChainDetails.surfaceCapabilities.maxImageCount < imageCount)
	{
		imageCount = swapChainDetails.surfaceCapabilities.maxImageCount;
	}

	//creation info for our swapchain
	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = instance.surface;
	swapChainCreateInfo.imageFormat = surfaceFormat.format;
	swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapChainCreateInfo.presentMode = presentMode;
	swapChainCreateInfo.imageExtent = extent;
	swapChainCreateInfo.minImageCount = imageCount;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapChainCreateInfo.preTransform = swapChainDetails.surfaceCapabilities.currentTransform;
	//handles blending between different graphics e.g windows
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	//clip part sof image that are off screen or blocked
	swapChainCreateInfo.clipped = VK_TRUE;

	//get queue family indices
	QueueFamilyIndices indices = GetQueueFamilies(device.physicalDevice, instance.surface);

	//if graphics and presentation families are different then swapchain must let images be shared between families
	if (indices.graphicsFamily != indices.presentationFamily)
	{
		uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentationFamily };

		//image share handling
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		//number of queues to share images
		swapChainCreateInfo.queueFamilyIndexCount = 2;
		//array of queues to share between
		swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.queueFamilyIndexCount = 0;
		swapChainCreateInfo.pQueueFamilyIndices = nullptr;
	}

	//if old swapchain been destroyed, and this one replaces it, 
	//then link old one and quickly hand over responsibilities
	swapChainCreateInfo.oldSwapchain = oldSwapchain;

	//create swapchain
	VkResult result = vkCreateSwapchainKHR(device.logicalDevice, &swapChainCreateInfo, nullptr, &swapchain);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Swapchain!");
	}
	VK_NAME(device.logicalDevice, "Swapchain", swapchain);

	//store for later reference
	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;

	//get swap chain images
	uint32_t swapChainImageCount;
	vkGetSwapchainImagesKHR(device.logicalDevice, swapchain, &swapChainImageCount, nullptr);
	std::vector<VkImage> images(swapChainImageCount);
	vkGetSwapchainImagesKHR(device.logicalDevice, swapchain, &swapChainImageCount, images.data());

	for (VkImage image : images)
	{
		//store image handles
		SwapChainImage swapChainImage = {};
		swapChainImage.image = image;
		swapChainImage.imageView = CreateImageView(device,image, swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

		//add to swapchain image list
		swapChainImages.push_back(swapChainImage);
	}

	CreateDepthBuffer();
	
	if (oldSwapchain)
	{
		if (oldSwapchain)
		{
			vkDestroySwapchainKHR(m_devicePtr->logicalDevice,oldSwapchain, nullptr);
		}
	}
}

void VulkanSwapchain::CreateDepthBuffer()
{
	if (depthAttachment.image)
	{
		depthAttachment.destroy(m_devicePtr->logicalDevice);
	}
	VkFormat depF = oGFX::ChooseSupportedFormat(*m_devicePtr,
		{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	depthAttachment.createAttachment(*m_devicePtr, swapChainExtent.width,  swapChainExtent.height,
		depF, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
}