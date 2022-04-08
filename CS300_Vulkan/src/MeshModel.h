#pragma once

#include <vulkan/vulkan.h>
#include "glm/glm.hpp"
#include "VulkanUtils.h"
#include "Mesh.h"

#pragma warning( push )
#pragma warning( disable : 26451 ) // vendor overflow
#include "assimp/scene.h"
#pragma warning( pop )

class MeshContainer
{
public:
	MeshContainer();
	MeshContainer(std::vector<Mesh> newMeshList);

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

	~MeshContainer();

private:
	std::vector<Mesh> meshList;
	glm::mat4 model{ 1.0f };
};

struct Model
{
	~Model();

	struct Vertices {
		int count;
		VkBuffer buffer;
		VkDeviceMemory memory;
	} vertices;
	struct Indices {
		int count;
		VkBuffer buffer;
		VkDeviceMemory memory;
	} indices;

	struct Textures
	{
		uint32_t albedo;
	}textures;

	void loadNode(Node* parent,const aiScene* scene, const aiNode& node, uint32_t nodeIndex
		,std::vector<oGFX::Vertex>& vertices, std::vector<uint32_t>& indices);

	VulkanDevice* device;
	std::vector<Node*> nodes;
private:
	oGFX::Mesh* processMesh(aiMesh* mesh, const aiScene* scene, std::vector<oGFX::Vertex>& vertices, std::vector<uint32_t>& indices);

};

