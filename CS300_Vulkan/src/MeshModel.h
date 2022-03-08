#pragma once

#include <vulkan/vulkan.h>
#include "glm/glm.hpp"
#include "VulkanUtils.h"
#include "Mesh.h"
#include "assimp/scene.h"

class MeshModel
{
public:
	MeshModel();
	MeshModel(std::vector<Mesh> newMeshList);

	size_t getMeshCount();
	Mesh *getMesh(size_t index);

	const glm::mat4& getModel();
	void setModel(glm::mat4 newModel);

	void destroyMeshModel();

	static std::vector<std::string> LoadMaterials(const aiScene *scene);
	static std::vector<Mesh> LoadNode(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool commandPool,
		aiNode *node, const aiScene *scene, std::vector<int> matToTex);
	static Mesh LoadMesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool commandPool,
		aiMesh *mesh, const aiScene *scene, std::vector<int> matToTex);

	~MeshModel();

private:
	std::vector<Mesh> meshList;
	glm::mat4 model{ 1.0f };
};

