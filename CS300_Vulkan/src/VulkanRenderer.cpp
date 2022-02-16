#include "VulkanRenderer.h"

#include <vector>
#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"
#include <stdexcept>

#include "Window.h"

void VulkanRenderer::CreateInstance(const oGFX::SetupInfo& setupSpecs)
{
	try
	{
		m_instance.Init(setupSpecs);
	}
	catch(...){
		throw;
	}
}

void VulkanRenderer::CreateSurface(const Window& window)
{
    //
    // Create the surface
    //

		// Get the Surface creation extension since we are about to use it
	auto VKCreateWin32Surface = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(m_instance.instance, "vkCreateWin32SurfaceKHR");
	if (nullptr == VKCreateWin32Surface)
	{
		throw std::runtime_error("Vulkan Driver missing the VK_KHR_win32_surface extension");		
    }

    VkWin32SurfaceCreateInfoKHR SurfaceCreateInfo
    {
    .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR
    ,   .pNext = nullptr
    ,   .flags = 0
    ,   .hinstance = GetModuleHandle(NULL)
    ,   .hwnd = reinterpret_cast<HWND>(window.GetRawHandle())
	};

	if (auto VKErr = VKCreateWin32Surface(m_instance.instance, &SurfaceCreateInfo, nullptr, &m_surface); VKErr)
	{
		throw std::runtime_error("Vulkan Fail to create window surface");
	}

}

void VulkanRenderer::AcquirePhysicalDevice()
{
    // Enumerate over physical devices the VK instance can access
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance.instance, &deviceCount, nullptr);

    // If no devices available, then none support vulkan!
    if (deviceCount == 0)
    {
        throw std::runtime_error("Can't find GPUs that support vulkan instance!");
    }

    // Get ist of physical devices
    std::vector<VkPhysicalDevice> deviceList(deviceCount);
    vkEnumeratePhysicalDevices(m_instance.instance, &deviceCount, deviceList.data());

    // find a suitable device
    for (const auto &device : deviceList)
    {
        if (CheckDeviceSuitable(device))
        {
            //found a nice device
            m_device.physicalDevice = device;
            break;
        }
    }
}

bool VulkanRenderer::CheckDeviceSuitable(VkPhysicalDevice device)
{
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    QueueFamilyIndices indices = GetQueueFamilies(device);

    bool extensionsSupported = CheckDeviceExtensionSupport(device);

    bool swapChainValid = false;
    if (extensionsSupported)
    {
        SwapChainDetails swapChainDetails = GetSwapchainDetails(device);
        swapChainValid = !swapChainDetails.presentationModes.empty() && !swapChainDetails.formats.empty();
    }
    return indices.isValid() && extensionsSupported && swapChainValid && deviceFeatures.samplerAnisotropy;
}

VulkanRenderer::QueueFamilyIndices VulkanRenderer::GetQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    //get the queue family properties for the device
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

    //go through each queue family and check if it has at least one of the required types of queue
    // it is possible to have a graphics queue family with no queues in it
    int i = 0;
    for (const auto &queueFamily : queueFamilyList)
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
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentationSupport);
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

bool VulkanRenderer::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
    // get device extension count
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    //if no devices return failure
    if (extensionCount == 0)
    {
        return false;
    }

    //populate list of extensions
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

    static const std::vector<const char*>deviceExtensions   = 
    { 
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
        //        ,   VK_NV_GLSL_SHADER_EXTENSION_NAME  // nVidia useful extension to be able to load GLSL shaders
    };

    //check extensions
    for (const auto &deviceExtension : deviceExtensions)
    {
        bool hasExtension = false;
        for (const auto &extension : extensions)
        {
            if (strcmp(deviceExtension, extension.extensionName) == 0)
            {
                hasExtension = true;
                break;
            }
        }
        if (!hasExtension)
        {
            return false;
        }
    }

    return true;
}

VulkanRenderer::SwapChainDetails VulkanRenderer::GetSwapchainDetails(VkPhysicalDevice device)
{
    VulkanRenderer::SwapChainDetails swapChainDetails;

    //CAPABILITIES
    //get the surface capabilities for the given surface on the given physical device
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface , &swapChainDetails.surfaceCapabilities);

    // FORMATS
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

    //if formats returned get the list of formats
    if (formatCount != 0)
    {
        swapChainDetails.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, swapChainDetails.formats.data());
    }

    // PRESENTATION MODES
    uint32_t presentationCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentationCount, nullptr);

    //if presentation modes returned get list of presentation modes.
    if (presentationCount != 0)
    {
        swapChainDetails.presentationModes.resize(presentationCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentationCount, swapChainDetails.presentationModes.data());
    }

    return swapChainDetails;
}
