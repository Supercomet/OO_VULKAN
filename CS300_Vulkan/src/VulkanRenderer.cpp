#include "VulkanRenderer.h"

#include <vector>
#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"

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
