#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanUtils.h"

class Mesh
{
public:
	Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice,
		VkQueue transferQueue,VkCommandPool transferCommandPool,
		std::vector<oGFX::Vertex> *vertices, std::vector<uint32_t> *indices, int newTexId);

	~Mesh();

	void SetTransform(oGFX::mat4 newModel);
	const oGFX::mat4& GetTransform();

	int getTexId();

	int getVertexCount();
	VkBuffer getVertexBuffer();

	int getIndexCount();
	VkBuffer getIndexBuffer();

	void destroyBuffers();


private:

	oGFX::mat4 model;
	int texId;

	int vertexCount;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	int indexCount;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	VkPhysicalDevice physicalDevice;
	VkDevice device;

	void CreateVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<oGFX::Vertex> *vertices);
	void CreateIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t> *indices);
};

