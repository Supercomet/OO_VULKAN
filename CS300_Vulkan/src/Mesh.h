#pragma once

#include <vulkan/vulkan.h>
#include "glm/glm.hpp"
#include <vector>
#include "VulkanUtils.h"

class Mesh
{
public:
	Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice,
		VkQueue transferQueue,VkCommandPool transferCommandPool,
		std::vector<oGFX::Vertex> *vertices, std::vector<uint32_t> *indices, int newTexId);

	~Mesh();

	void SetTransform(glm::mat4 newModel);
	const glm::mat4& GetTransform();

	int getTexId();

	int getVertexCount();
	VkBuffer getVertexBuffer();

	int getIndexCount();
	VkBuffer getIndexBuffer();

	void destroyBuffers();


private:

	glm::mat4 model{ 1.0f };
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

