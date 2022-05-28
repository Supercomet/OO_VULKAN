#if defined(_WIN32)
#define NOMINMAX
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
    if (commandPool)
    {
        vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
    }
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

    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    //get the queue family indices for the chosen Physical Device
    queueIndices = oGFX::GetQueueFamilies(physicalDevice, instance.surface);
    oGFX::QueueFamilyIndices& indices = queueIndices;

    //vector for queue creation information and set for family indices
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> queueFamilyIndices = { indices.graphicsFamily,indices.presentationFamily };

    //queues the logical device needs to create in the info to do so.
    for (int queueFamilyIndex : queueFamilyIndices)
    {
        (void)queueFamilyIndex;
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
    deviceFeatures.multiDrawIndirect = VK_TRUE;

    deviceFeatures.fillModeNonSolid = VK_TRUE;  //wireframe drawing
    
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    // Bindless design requirement Descriptor indexing for descriptors
    VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features{};
    descriptor_indexing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    // Enable non-uniform indexing
    descriptor_indexing_features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    descriptor_indexing_features.runtimeDescriptorArray = VK_TRUE;
    descriptor_indexing_features.descriptorBindingVariableDescriptorCount = VK_TRUE;
    descriptor_indexing_features.descriptorBindingPartiallyBound = VK_TRUE;


    deviceCreateInfo.pNext = &descriptor_indexing_features;

    this->enabledFeatures = deviceFeatures;

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


    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = indices.graphicsFamily; //Queue family type that buffers from this command pool will use

                                                                   //create a graphics queue family command pool
    result = vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to createa a command pool!");
    }

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
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
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
            //TODO: throw what extension not supported
            return false;
        }
    }

    return true;
}

VkResult VulkanDevice::CreateBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, vk::Buffer* buffer, VkDeviceSize size, void* data)
{
    buffer->device = logicalDevice;

    // Create the buffer handle
    VkBufferCreateInfo bufferCreateInfo = oGFX::vk::inits::bufferCreateInfo(usageFlags, size);
    vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, &buffer->buffer);

    // Create the memory backing up the buffer handle
    VkMemoryRequirements memReqs;
    VkMemoryAllocateInfo memAlloc = oGFX::vk::inits::memoryAllocateInfo();
    vkGetBufferMemoryRequirements(logicalDevice, buffer->buffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    // Find a memory type index that fits the properties of the buffer
    memAlloc.memoryTypeIndex = oGFX::FindMemoryTypeIndex(physicalDevice,memReqs.memoryTypeBits, memoryPropertyFlags);
    // If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set we also need to enable the appropriate flag during allocation
    VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
    if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
        allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
        memAlloc.pNext = &allocFlagsInfo;
    }
    vkAllocateMemory(logicalDevice, &memAlloc, nullptr, &buffer->memory);

    buffer->alignment = memReqs.alignment;
    buffer->size = size;
    buffer->usageFlags = usageFlags;
    buffer->memoryPropertyFlags = memoryPropertyFlags;

    // If a pointer to the buffer data has been passed, map the buffer and copy over the data
    if (data != nullptr)
    {
        (buffer->map());
        memcpy(buffer->mapped, data, size);
        if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
            buffer->flush();

        buffer->unmap();
    }

    // Initialize a default descriptor that covers the whole buffer size
    buffer->setupDescriptor();

    // Attach the memory to the buffer object
    return buffer->bind();
}

VkCommandBuffer VulkanDevice::CreateCommandBuffer(VkCommandBufferLevel level, VkCommandPool pool, bool begin)
{
    VkCommandBufferAllocateInfo cmdBufAllocateInfo = oGFX::vk::inits::commandBufferAllocateInfo(pool, level, 1);
    VkCommandBuffer cmdBuffer;
    vkAllocateCommandBuffers(logicalDevice, &cmdBufAllocateInfo, &cmdBuffer);
    // If requested, also start recording for the new command buffer
    if (begin)
    {
        VkCommandBufferBeginInfo cmdBufInfo = oGFX::vk::inits::commandBufferBeginInfo();
        vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo);
    }
    return cmdBuffer;
}

VkCommandBuffer VulkanDevice::CreateCommandBuffer(VkCommandBufferLevel level, bool begin)
{
    return CreateCommandBuffer(level, commandPool, begin);
}

void VulkanDevice::FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free)
{
    if (commandBuffer == VK_NULL_HANDLE)
    {
        return;
    }

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = oGFX::vk::inits::submitInfo();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    // Create fence to ensure that the command buffer has finished executing
    VkFenceCreateInfo fenceInfo = oGFX::vk::inits::fenceCreateInfo(0);
    VkFence fence;
    vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fence);
    // Submit to the queue
    vkQueueSubmit(queue, 1, &submitInfo, fence);
    // Wait for the fence to signal that command buffer has finished executing
    vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE, 100000000000);
    vkDestroyFence(logicalDevice, fence, nullptr);
    if (free)
    {
        vkFreeCommandBuffers(logicalDevice, pool, 1, &commandBuffer);
    }
}

void VulkanDevice::FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free)
{
    return FlushCommandBuffer(commandBuffer, queue, commandPool, free);
}

void VulkanDevice::CopyBuffer(vk::Buffer* src, vk::Buffer* dst, VkQueue queue, VkBufferCopy* copyRegion)
{
    assert(dst->size <= src->size);
    assert(src->buffer);
    VkCommandBuffer copyCmd = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    VkBufferCopy bufferCopy{};
    if (copyRegion == nullptr)
    {
        bufferCopy.size = src->size;
    }
    else
    {
        bufferCopy = *copyRegion;
    }

    vkCmdCopyBuffer(copyCmd, src->buffer, dst->buffer, 1, &bufferCopy);

    FlushCommandBuffer(copyCmd, queue);
}


