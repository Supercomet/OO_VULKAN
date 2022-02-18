#if defined(_WIN32)
#include <windows.h>
#endif

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"
#include "VulkanDevice.h"

#include "VulkanInstance.h"
#include "Window.h"

#include <vector>
#include <stdexcept>
#include <set>

VulkanDevice::~VulkanDevice()
{
    // no need destory phys device
	if (logicalDevice)
	{
		vkDestroyDevice(logicalDevice,NULL);
		logicalDevice = NULL;
	}
}

void VulkanDevice::InitPhysicalDevice(VulkanInstance& instance)
{
    m_instancePtr = &instance;
    // Enumerate over physical devices the VK instance can access
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance.instance, &deviceCount, nullptr);

    // If no devices available, then none support vulkan!
    if (deviceCount == 0)
    {
        throw std::runtime_error("Can't find GPUs that support vulkan instance!");
    }

    // Get ist of physical devices
    std::vector<VkPhysicalDevice> deviceList(deviceCount);
    vkEnumeratePhysicalDevices(instance.instance, &deviceCount, deviceList.data());

    // find a suitable device
    for (const auto &device : deviceList)
    {
        if (CheckDeviceSuitable(device))
        {
            //found a nice device
            physicalDevice = device;
            break;
        }
    }
}

void VulkanDevice::InitLogicalDevice(VulkanInstance& instance)
{
    //get the queue family indices for the chosen Physical Device
    oGFX::QueueFamilyIndices indices = oGFX::GetQueueFamilies(physicalDevice, instance.surface);

    //vector for queue creation information and set for family indices
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> queueFamilyIndices = { indices.graphicsFamily,indices.presentationFamily };

    //queues the logical device needs to create in the info to do so.
    for (int queueFamilyIndex : queueFamilyIndices)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        //the index of the family to create a queue from
        queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;
        //number of queues to create
        queueCreateInfo.queueCount = 1;
        //vulkan needs to know how to handle multiple queues and thus we need a priority, 1.0 is the highest priority
        float priority = 1.0f;
        queueCreateInfo.pQueuePriorities = &priority;

        queueCreateInfos.push_back(queueCreateInfo);
    }

    std::vector<const char*>deviceExtensions{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    //information to create logical device (somtimes called only "device")
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    //queue create info so the device can create required queues
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    //number of enabled logical device extensions
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    //all features will be disabled by default
    VkPhysicalDeviceFeatures deviceFeatures = {};
    //physical device features that logical device will use
    deviceFeatures.samplerAnisotropy = VK_TRUE; // Enabling anisotropy

    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    // TODO: memory management
    // Create logical device for the given physical device
    VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("VK Renderer Failed to create a logical device!");
    }

    // Queues are created as the same time as the device...
    // So we want to handle the queues
    // From given logical device of given queue family of given index, place reference in VKqueue
    vkGetDeviceQueue(logicalDevice, indices.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(logicalDevice, indices.presentationFamily, 0, &presentationQueue);
}


bool VulkanDevice::CheckDeviceSuitable(VkPhysicalDevice device)
{
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	oGFX::QueueFamilyIndices indices = oGFX::GetQueueFamilies(device,m_instancePtr->surface);

	bool extensionsSupported = CheckDeviceExtensionSupport(device);

	bool swapChainValid = false;
	if (extensionsSupported)
	{
		oGFX::SwapChainDetails swapChainDetails = oGFX::GetSwapchainDetails(*m_instancePtr,device);
		swapChainValid = !swapChainDetails.presentationModes.empty() && !swapChainDetails.formats.empty();
	}
	return indices.isValid() && extensionsSupported && swapChainValid && deviceFeatures.samplerAnisotropy;
}


bool VulkanDevice::CheckDeviceExtensionSupport(VkPhysicalDevice device)
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


