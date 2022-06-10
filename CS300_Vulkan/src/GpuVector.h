#pragma once
#include "vulkan/vulkan.h"

struct VulkanDevice;

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
