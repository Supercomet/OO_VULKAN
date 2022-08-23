#pragma once
#include "vulkan/vulkan.h"
#include <vector>


class DescriptorAllocator {
public:

	struct PoolSizes {
		std::vector<std::pair<VkDescriptorType,float>> sizes =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f }
		};
	};

	void ResetPools();
	bool Allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout);

	void Init(VkDevice newDevice);

	void Cleanup();

	VkDevice device{};
private:
	VkDescriptorPool GrabPool();
	VkDescriptorPool CreatePool(VkDevice device, const DescriptorAllocator::PoolSizes& poolSizes, int count, VkDescriptorPoolCreateFlags flags);

	VkDescriptorPool currentPool{VK_NULL_HANDLE};
	PoolSizes descriptorSizes;
	std::vector<VkDescriptorPool> usedPools;
	std::vector<VkDescriptorPool> freePools;
};