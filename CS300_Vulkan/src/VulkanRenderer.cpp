#include "VulkanRenderer.h"

#include <vector>
#include <set>
#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"
#include <stdexcept>

#include "VulkanUtils.h"
#include "Window.h"


VulkanRenderer::~VulkanRenderer()
{
   
}

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

void VulkanRenderer::CreateSurface(Window& window)
{
    windowPtr = &window;
    m_instance.CreateSurface(window);
}

void VulkanRenderer::AcquirePhysicalDevice()
{
    m_device.InitPhysicalDevice(m_instance);
}


void VulkanRenderer::CreateLogicalDevice()
{
    m_device.InitLogicalDevice(m_instance);
}


