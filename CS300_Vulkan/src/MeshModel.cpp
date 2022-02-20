#include "MeshModel.h"

#include <stdexcept>

MeshModel::MeshModel()
{
}

MeshModel::MeshModel(std::vector<Mesh> newMeshList)
{
	meshList = newMeshList;
	model = oGFX::mat4();
}

size_t MeshModel::getMeshCount()
{
	return meshList.size();
}

Mesh* MeshModel::getMesh(size_t index)
{
	if (index >= meshList.size())
	{
		throw std::runtime_error("Attempted to access invad mesh index!");
	}

	return &meshList[index];
}

const oGFX::mat4& MeshModel::getModel()
{
	return model;
}

void MeshModel::setModel(oGFX::mat4 newModel)
{
	model = newModel;
}

void MeshModel::destroyMeshModel()
{
	for (auto &mesh : meshList)
	{
		mesh.destroyBuffers();
	}
}

MeshModel::~MeshModel()
{
}
