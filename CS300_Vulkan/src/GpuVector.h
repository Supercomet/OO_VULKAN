#pragma once
#include "vulkan/vulkan.h"

struct VulkanDevice;

template <typename T>
class GpuVector{
public:
	GpuVector(VulkanDevice* device);
	void Init(VkBufferUsageFlags usage);

	void writeTo(size_t size, void* data, size_t offset = 0);

	void resize(size_t size);
	void reserve(size_t size);
	size_t size() const;

	VkBuffer getBuffer()const;
	const VkBuffer* getBufferPtr()const;

	void destroy();
	void clear();

private:
	size_t m_size{};
	size_t m_capacity{};
	VkBufferUsageFlags m_usage{};
	VkBuffer m_buffer{};
	VkDeviceMemory m_gpuMemory{};

	VulkanDevice* m_device{};

};


#ifndef GPU_VECTOR_CPP
#define GPU_VECTOR_CPP
#include "GpuVector.h"
#include "VulkanUtils.h"
#include "VulkanDevice.h"

template <typename T>
GpuVector<T>::GpuVector(VulkanDevice* device) :
	m_device{ device }
{

}

template <typename T>
void GpuVector<T>::Init(VkBufferUsageFlags usage)
{
	m_usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	oGFX::CreateBuffer(m_device->physicalDevice, m_device->logicalDevice, 1, m_usage,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_buffer, &m_gpuMemory);
}

template <typename T>
void GpuVector<T>::writeTo(size_t writeSize, void* data, size_t offset)
{
	if ((writeSize + offset) > m_capacity)
	{
		// TODO:  maybe resize some amount instead of perfect amount?
		resize(m_capacity+ writeSize + offset);
	}

	using namespace oGFX;
	//get writeSize of buffer needed for vertices
	VkDeviceSize bufferSize = writeSize*sizeof(T);
	VkDeviceSize writeOffset = offset * sizeof(T);

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
		stagingBuffer, m_buffer, bufferSize, writeOffset);

	//clean up staging buffer parts
	vkDestroyBuffer(m_device->logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(m_device->logicalDevice, stagingBufferMemory, nullptr);

	//not sure what to do here, we just assume that its tightly packed
	if (offset < m_size)
	{
		m_size -= m_size - offset;
	}
	m_size += writeSize;

}

template <typename T>
void GpuVector<T>::resize(size_t size)
{
	reserve(size);
	m_size = size;	
}

template <typename T>
void GpuVector<T>::reserve(size_t size)
{

	if (size < m_capacity) return;

	using namespace oGFX;
	VkDeviceSize bufferSize = size * sizeof(T);

	if (bufferSize == 0) return;

	VkBuffer tempBuffer;
	VkDeviceMemory tempMemory;
	CreateBuffer(m_device->physicalDevice, m_device->logicalDevice, bufferSize, m_usage,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &tempBuffer, &tempMemory); // VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT make this buffer local to the GPU
																		//copy staging buffer to vertex buffer on GPU
	if (m_size != 0)
	{
		CopyBuffer(m_device->logicalDevice, m_device->graphicsQueue, m_device->commandPool, m_buffer, tempBuffer, m_size* sizeof(T));
	}

	//clean up old buffer
	vkDestroyBuffer(m_device->logicalDevice, m_buffer, nullptr);
	vkFreeMemory(m_device->logicalDevice, m_gpuMemory, nullptr);

	m_buffer = tempBuffer;
	m_gpuMemory = tempMemory;

	m_capacity = size;
}

template <typename T>
size_t GpuVector<T>::size() const
{
	return m_size;
}
template <typename T>
VkBuffer GpuVector<T>::getBuffer() const
{
	return m_buffer;
}
template <typename T>
const VkBuffer* GpuVector<T>::getBufferPtr() const
{
	return &m_buffer;
}
template <typename T>
void GpuVector<T>::destroy()
{
	//clean up old buffer
	vkDestroyBuffer(m_device->logicalDevice, m_buffer, nullptr);
	vkFreeMemory(m_device->logicalDevice, m_gpuMemory, nullptr);
}
template <typename T>
void GpuVector<T>::clear()
{
	m_size = 0;
}

#endif // !GPU_VECTOR_CPP

