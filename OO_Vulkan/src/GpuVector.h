/************************************************************************************//*!
\file           GpuVector.h
\project        Ouroboros
\author         Jamie Kong, j.kong, 390004720 | code contribution (100%)
\par            email: j.kong\@digipen.edu
\date           Oct 02, 2022
\brief              Defines a wrapper object for resiable GPU buffer. Generally bad idea

Copyright (C) 2022 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*//*************************************************************************************/
#pragma once
#include "vulkan/vulkan.h"
#include <iostream>
#include "Profiling.h"
#include "DelayedDeleter.h"

struct VulkanDevice;

extern uint64_t accumulatedBytes;

template <typename T>
class GpuVector {
public:
	GpuVector();
	GpuVector(VulkanDevice* device);
	void Init(VkBufferUsageFlags usage);
	void Init(VulkanDevice* device, VkBufferUsageFlags usage);

	void blockingWriteTo(size_t size, const void* data, VkQueue queue, VkCommandPool pool, size_t offset = 0);
	void writeToCmd(size_t writeSize, const void* data, VkCommandBuffer command, VkQueue queue, VkCommandPool pool, size_t offset = 0);

	void addWriteCommand(size_t size, const void* data, size_t offset = 0);
	void flushToGPU(VkCommandBuffer command, VkQueue queue, VkCommandPool pool);

	void resize(size_t size, VkQueue queue, VkCommandPool pool);
	void reserve(size_t size, VkQueue queue, VkCommandPool pool);
	size_t size() const;

	VkBuffer getBuffer()const;
	const VkBuffer* getBufferPtr()const;

	void destroy();
	void clear();

	const VkDescriptorBufferInfo& GetDescriptorBufferInfo();
	const VkDescriptorBufferInfo* GetBufferInfoPtr();
	bool MustUpdate();
	void Updated();

public:
	size_t m_size{ 0 };
	size_t m_capacity{ 0 };
	VkBufferUsageFlags m_usage{};
	VkBuffer m_buffer{ VK_NULL_HANDLE };
	VkDeviceMemory m_gpuMemory{ VK_NULL_HANDLE };
	VkDescriptorBufferInfo m_descriptor{};

	VulkanDevice* m_device{ nullptr };

	bool m_mustUpdate;
	std::vector<VkBufferCopy>m_copyRegions;
	std::vector<T>m_cpuBuffer;
};

#ifndef GPU_VECTOR_CPP
#define GPU_VECTOR_CPP
#include "GpuVector.h"
#include "VulkanUtils.h"
#include "VulkanDevice.h"

class VulkanRenderer;
template<typename T>
GpuVector<T>::GpuVector() : 
	m_device{nullptr},
	m_buffer{VK_NULL_HANDLE}
{

}

template <typename T>
GpuVector<T>::GpuVector(VulkanDevice* device) :
	m_device{ device },
	m_buffer{VK_NULL_HANDLE}
{

}

template <typename T>
void GpuVector<T>::Init(VkBufferUsageFlags usage)
{
	assert(m_device != nullptr); // invalid device ptr. or didnt provide
	assert(m_buffer == VK_NULL_HANDLE); // called init twice
	assert(m_gpuMemory == VK_NULL_HANDLE); // called init twice
	m_usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	oGFX::CreateBuffer(m_device->physicalDevice, m_device->logicalDevice, 1, m_usage,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_buffer, &m_gpuMemory);
}

template<typename T>
inline void GpuVector<T>::Init(VulkanDevice* device, VkBufferUsageFlags usage)
{
	m_device = device;
	Init(usage);
}

template <typename T>
void GpuVector<T>::blockingWriteTo(size_t writeSize,const void* data, VkQueue queue, VkCommandPool pool, size_t offset)
{
	if (writeSize == 0) 
		return;
	PROFILE_SCOPED();
	if ((writeSize + offset) > m_capacity)
	{
		// TODO:  maybe resize some amount instead of perfect amount?
		assert(true);
		resize(m_capacity?m_capacity*2 : 64, queue, pool);
		blockingWriteTo(writeSize, data, queue, pool, offset);
		return;
	}

	
	using namespace oGFX;
	//get writeSize of buffer needed for vertices
	VkDeviceSize bufferBytes = writeSize*sizeof(T);
	VkDeviceSize writeBytesOffset = offset * sizeof(T);

	//temporary buffer to stage vertex data before transferring to GPU
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory; 

	//create buffer and allocate memory to it
	CreateBuffer(m_device->physicalDevice, m_device->logicalDevice, bufferBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

	//MAP MEMORY TO VERTEX BUFFER
	void *mappedData = nullptr;												
	auto result = vkMapMemory(m_device->logicalDevice, stagingBufferMemory, 0, bufferBytes, 0, &mappedData);
	if (result != VK_SUCCESS)
	{
		assert(false);
	}
	memcpy(mappedData, data, (size_t)bufferBytes);					
	vkUnmapMemory(m_device->logicalDevice, stagingBufferMemory);					

	CopyBuffer(m_device->logicalDevice, queue,pool,
		stagingBuffer, m_buffer, bufferBytes, writeBytesOffset);


	{
		PROFILE_SCOPED("Clean buffer") 

		//clean up staging buffer parts
		vkDestroyBuffer(m_device->logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(m_device->logicalDevice, stagingBufferMemory, nullptr);
	}

	//not sure what to do here, we just assume that its tightly packed
	if (offset < m_size)
	{
		m_size -= m_size - offset;
	}
	m_size += writeSize;

}

template <typename T>
void GpuVector<T>::writeToCmd(size_t writeSize, const void* data,VkCommandBuffer command, VkQueue queue,VkCommandPool pool, size_t offset)
{
	if (writeSize == 0)
		return;
	PROFILE_SCOPED();
	if ((writeSize + offset) > m_capacity)
	{
		// TODO:  maybe resize some amount instead of perfect amount?
		assert(true);
		resize(m_capacity ? m_capacity * 2 : 64, queue, pool);
		writeToCmd(writeSize, data, command, queue, pool, offset);
		return;
	}


	using namespace oGFX;
	//get writeSize of buffer needed for vertices
	VkDeviceSize bufferBytes = writeSize * sizeof(T);
	VkDeviceSize writeBytesOffset = offset * sizeof(T);

	//temporary buffer to stage vertex data before transferring to GPU
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	//create buffer and allocate memory to it
	CreateBuffer(m_device->physicalDevice, m_device->logicalDevice, bufferBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

	//MAP MEMORY TO VERTEX BUFFER
	void* mappedData = nullptr;
	auto result = vkMapMemory(m_device->logicalDevice, stagingBufferMemory, 0, bufferBytes, 0, &mappedData);
	if (result != VK_SUCCESS)
	{
		assert(false);
	}
	memcpy(mappedData, data, (size_t)bufferBytes);
	vkUnmapMemory(m_device->logicalDevice, stagingBufferMemory);

	
	auto commandBuffer = command;
	
	// region of data to copy from and to
	VkBufferCopy bufferCopyRegion{};
	bufferCopyRegion.srcOffset = 0;
	bufferCopyRegion.dstOffset = 0;
	bufferCopyRegion.size = bufferBytes;
	
	// command to copy src buffer to dst buffer
	vkCmdCopyBuffer(commandBuffer, stagingBuffer, m_buffer, 1, &bufferCopyRegion);
	
	auto fun = [oldBuffer = stagingBuffer, logical = m_device->logicalDevice, oldMemory = stagingBufferMemory]() {
		PROFILE_SCOPED("Clean buffer")
		//clean up staging buffer parts
		vkFreeMemory(logical, oldMemory, nullptr);
		vkDestroyBuffer(logical, oldBuffer, nullptr);
	};
	DelayedDeleter::get()->DeleteAfterFrames(fun);

	//not sure what to do here, we just assume that its tightly packed
	if (offset < m_size)
	{
		m_size -= m_size - offset;
	}
	m_size += writeSize;

}

template<typename T>
inline void GpuVector<T>::addWriteCommand(size_t writeSize, const void* data, size_t offset)
{
	VkDeviceSize dataBytes = writeSize * sizeof(T);
	VkDeviceSize writeBytesOffset = offset * sizeof(T);

	auto oldSize = m_cpuBuffer.size();
	VkDeviceSize cpuBytesOffset = oldSize * sizeof(T);

	VkBufferCopy bufferCopyRegion{};
	bufferCopyRegion.srcOffset = cpuBytesOffset;
	bufferCopyRegion.dstOffset = writeBytesOffset;
	bufferCopyRegion.size = dataBytes;

	m_copyRegions.push_back(bufferCopyRegion);
	
	m_cpuBuffer.resize(oldSize + writeSize);
	memcpy(m_cpuBuffer.data() + oldSize, data, dataBytes);

	m_mustUpdate = true;

}

template<typename T>
inline void GpuVector<T>::flushToGPU(VkCommandBuffer command, VkQueue queue, VkCommandPool pool)
{
	VkDeviceSize totalDataSize{};
	size_t largestWrite{};
	for (const auto& copycmd: m_copyRegions)
	{
		totalDataSize += copycmd.size;
		largestWrite = std::max(largestWrite, copycmd.dstOffset + copycmd.size);
	}
	size_t maxElement = largestWrite / sizeof(T);

	if (totalDataSize == 0)
	{
		return;
	}

	PROFILE_SCOPED();
	if ((maxElement) > m_capacity)
	{
		assert(true);
		resize(maxElement, queue, pool);
	}

	//temporary buffer to stage vertex data before transferring to GPU
	VkBuffer stagingBuffer{};
	VkDeviceMemory stagingBufferMemory{};

	//create buffer and allocate memory to it
	oGFX::CreateBuffer(m_device->physicalDevice, m_device->logicalDevice, totalDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

	//MAP MEMORY TO VERTEX BUFFER
	void* mappedData = nullptr;
	auto result = vkMapMemory(m_device->logicalDevice, stagingBufferMemory, 0, totalDataSize, 0, &mappedData);
	if (result != VK_SUCCESS)
	{
		assert(false);
	}
	memcpy(mappedData, m_cpuBuffer.data(), (size_t)totalDataSize);
	vkUnmapMemory(m_device->logicalDevice, stagingBufferMemory);
	
	//m_cpuBuffer.clear(); // good for small memory..
	m_cpuBuffer = {}; // release the memory because it could be quite big

	auto commandBuffer = command;

	// command to copy src buffer to dst buffer
	vkCmdCopyBuffer(commandBuffer, stagingBuffer, m_buffer, m_copyRegions.size(), m_copyRegions.data());
	m_copyRegions.clear();

	auto fun = [oldBuffer = stagingBuffer, logical = m_device->logicalDevice, oldMemory = stagingBufferMemory]() {
		PROFILE_SCOPED("Clean buffer");
		//clean up staging buffer parts
		vkFreeMemory(logical, oldMemory, nullptr);
		vkDestroyBuffer(logical, oldBuffer, nullptr);
	};
	DelayedDeleter::get()->DeleteAfterFrames(fun);

	m_mustUpdate = false;
}

template <typename T>
void GpuVector<T>::resize(size_t size, VkQueue queue, VkCommandPool pool)
{
	std::cout << "[GpuVector<T>::resize] " << "Resizing from " << m_size << " to " << size << "\n";
	reserve(size, queue, pool);
	m_size = size;	
}

template <typename T>
void GpuVector<T>::reserve(size_t size, VkQueue queue, VkCommandPool pool)
{
	if (size <= m_capacity) return;
	PROFILE_SCOPED();

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
		CopyBuffer(m_device->logicalDevice, queue, pool, m_buffer, tempBuffer, m_size* sizeof(T));
	}

	auto fun = [oldBuffer = m_buffer, logical = m_device->logicalDevice, oldMemory = m_gpuMemory]() {
			PROFILE_SCOPED("Clean up buffer");
			vkDestroyBuffer(logical, oldBuffer, nullptr);
			vkFreeMemory(logical, oldMemory, nullptr);
		};
	DelayedDeleter::get()->DeleteAfterFrames(fun);


	m_buffer = tempBuffer;
	m_gpuMemory = tempMemory;

	// accumulate bytes
	accumulatedBytes -= m_capacity;
	accumulatedBytes += size;

	m_capacity = size;

	m_mustUpdate = true;
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
	if (m_buffer)
	{
		vkDestroyBuffer(m_device->logicalDevice, m_buffer, nullptr);
		vkFreeMemory(m_device->logicalDevice, m_gpuMemory, nullptr);
	}
}
template <typename T>
void GpuVector<T>::clear()
{
	m_size = 0;
}

template<typename T>
inline const VkDescriptorBufferInfo& GpuVector<T>::GetDescriptorBufferInfo()
{
	m_descriptor.buffer = m_buffer;
	m_descriptor.offset = 0;
	m_descriptor.range = VK_WHOLE_SIZE;
	return m_descriptor;
}

template<typename T>
inline const VkDescriptorBufferInfo* GpuVector<T>::GetBufferInfoPtr()
{
	m_descriptor.buffer = m_buffer;
	m_descriptor.offset = 0;
	m_descriptor.range = VK_WHOLE_SIZE;
	return &m_descriptor;
}

template<typename T>
inline bool GpuVector<T>::MustUpdate()
{
	return m_mustUpdate;
}

template<typename T>
inline void GpuVector<T>::Updated()
{
	m_mustUpdate = false;
}

#endif // !GPU_VECTOR_CPP

