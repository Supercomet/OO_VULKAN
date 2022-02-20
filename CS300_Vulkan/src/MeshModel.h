#pragma once

#include <vulkan/vulkan.h>
#include "VulkanUtils.h"
#include "Mesh.h"

class MeshModel
{
public:
	MeshModel();
	MeshModel(std::vector<Mesh> newMeshList);

	size_t getMeshCount();
	Mesh *getMesh(size_t index);

	const oGFX::mat4& getModel();
	void setModel(oGFX::mat4 newModel);

	void destroyMeshModel();

	~MeshModel();

private:
	std::vector<Mesh> meshList;
	oGFX::mat4 model;
};

