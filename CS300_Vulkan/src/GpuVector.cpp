#include "GpuVector.h"
#include "VulkanUtils.h"


#include "VulkanDevice.h"

GpuVector::GpuVector(VulkanDevice* device) :
	m_device{ device }
{
	
}

void GpuVector::Init(VkBufferUsageFlags usage)
{
	m_usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	oGFX::CreateBuffer(m_device->physicalDevice, m_device->logicalDevice, 1, m_usage,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_buffer, &m_gpuMemory);
}

void GpuVector::writeTo(size_t writeSize, void* data, size_t offset)
{
	if ((writeSize + offset) > m_capacity)
	{
		// TODO:  maybe resize some amount instead of perfect amount?
		resize(m_capacity+ writeSize + offset);
	}

	using namespace oGFX;
	//get writeSize of buffer needed for vertices
	VkDeviceSize bufferSize = writeSize;

	//temporary buffer to stage vertex data before transferring to GPU
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory; 

	//create buffer and allocate memory to it
	CreateBuffer(m_device->physicalDevice, m_device->logicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

	//MAP MEMORY TO VERTEX BUFFER
	void *mappedData = nullptr;												
	vkMapMemory(m_device->logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &mappedData);	
	memcpy(mappedData, data, (size_t)bufferSize);					
	vkUnmapMemory(m_device->logicalDevice, stagingBufferMemory);					

	CopyBuffer(m_device->logicalDevice, m_device->graphicsQueue, m_device->commandPool,
		stagingBuffer, m_buffer, bufferSize, offset);

	//clean up staging buffer parts
	vkDestroyBuffer(m_device->logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(m_device->logicalDevice, stagingBufferMemory, nullptr);

}

void GpuVector::resize(size_t size)
{
	reserve(size);
	m_size = size;	
}

void GpuVector::reserve(size_t size)
{

	if (size < m_capacity) return;

	using namespace oGFX;
	VkDeviceSize bufferSize = size;

	VkBuffer tempBuffer;
	VkDeviceMemory tempMemory;
	CreateBuffer(m_device->physicalDevice, m_device->logicalDevice, bufferSize, m_usage,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &tempBuffer, &tempMemory); // VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT make this buffer local to the GPU
																		//copy staging buffer to vertex buffer on GPU
	if (m_size != 0)
	{
		CopyBuffer(m_device->logicalDevice, m_device->graphicsQueue, m_device->commandPool, m_buffer, tempBuffer, m_size);
	}

	//clean up old buffer
	vkDestroyBuffer(m_device->logicalDevice, m_buffer, nullptr);
	vkFreeMemory(m_device->logicalDevice, m_gpuMemory, nullptr);

	m_buffer = tempBuffer;
	m_gpuMemory = tempMemory;

	m_capacity = size;
}

size_t GpuVector::size() const
{
	return m_size;
}

VkBuffer GpuVector::getBuffer() const
{
	return m_buffer;
}

const VkBuffer* GpuVector::getBufferPtr() const
{
	return &m_buffer;
}

void GpuVector::destroy()
{
	//clean up old buffer
	vkDestroyBuffer(m_device->logicalDevice, m_buffer, nullptr);
	vkFreeMemory(m_device->logicalDevice, m_gpuMemory, nullptr);
}

void GpuVector::clear()
{
	m_size = 0;
}
