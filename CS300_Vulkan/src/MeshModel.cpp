#include "MeshModel.h"

#include <stdexcept>

MeshModel::MeshModel()
{
}

MeshModel::MeshModel(std::vector<Mesh> newMeshList)
{
	meshList = newMeshList;
	model = glm::mat4(1.0f);
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

const glm::mat4& MeshModel::getModel()
{
	return model;
}

void MeshModel::setModel(glm::mat4 newModel)
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


std::vector<std::string> MeshModel::LoadMaterials(const aiScene *scene)
{
	// create 1:1 size list of textures
	std::vector <std::string> textureList(scene->mNumMaterials);

	// go through each material and copy its texxture file name( if it exists)
	for (size_t i = 0; i < scene->mNumMaterials; i++)
	{
		//get the material
		aiMaterial *material = scene->mMaterials[i];

		// Initialize the texture to empty string ( will be replaced if texture exists)
		textureList[i] = "";

		// Check for a diffuse texture (standard details)
		if (material->GetTextureCount(aiTextureType_DIFFUSE))
		{
			//get path of the texture file
			aiString path;
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path)== AI_SUCCESS)
			{
				// C:\users\jtk\documents\thing.obj
				// Cut off any directory information already present
				std::string str = std::string(path.data);
				size_t idx = str.rfind("\\");
				std::string filename = str.substr(idx + 1);

				textureList[i] = filename;
			}
		}
	}
	return textureList;
}

std::vector<Mesh> MeshModel::LoadNode(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool commandPool, aiNode *node, const aiScene *scene, std::vector<int> matToTex)
{
	std::vector<Mesh> meshList;

	//go through teach mesh at this node and create it , then add it to our meshlist
	for (size_t i = 0; i < node->mNumMeshes; i++)
	{
		meshList.push_back(LoadMesh(newPhysicalDevice, newDevice, transferQueue, commandPool,
			scene->mMeshes[node->mMeshes[i]], scene, matToTex));
	}

	// Go through each node attached to this node and load it then append their meshes to this nod's mesh list
	for (size_t i = 0; i < node->mNumChildren; i++)
	{
		std::vector<Mesh> newList = LoadNode(newPhysicalDevice,  newDevice,  transferQueue,  commandPool,
			node->mChildren[i], scene, matToTex);
		meshList.insert(meshList.end(), newList.begin(), newList.end());
	}
	return meshList;
}

Mesh MeshModel::LoadMesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool,
	aiMesh *mesh, const aiScene *scene, std::vector<int> matToTex)
{
	std::vector<oGFX::Vertex> vertices;
	std::vector<uint32_t> indices;

	// Resize vertex list to hold all vertices for mesh
	vertices.resize(mesh->mNumVertices);
	// Go through each vertex and copy it across to our vertices
	for (size_t i = 0; i < mesh->mNumVertices; i++)
	{
		// Set position
		vertices[i].pos = { mesh->mVertices[i].x ,mesh->mVertices[i].y,mesh->mVertices[i].z };

		// Set tex coords (if they exist)
		if (mesh->mTextureCoords[0])
		{
			vertices[i].tex = { mesh->mTextureCoords[0][i].x,mesh->mTextureCoords[0][i].y };
		}
		else
		{
			//default value
			vertices[i].tex = { 0.0f,0.0f };
		}

		// Set colour to white for now
		vertices[i].col = { 1.0f,1.0f,1.0f };
	}

	// Iterate over indices through ffaces and copy accross
	for (size_t i = 0; i < mesh->mNumFaces; i++)
	{
		// get a face
		aiFace face = mesh->mFaces[i];

		// go through face's indices and add to list
		for (size_t j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	// Create a new mesh with details and return it
	Mesh newMesh = Mesh(newPhysicalDevice, newDevice, transferQueue, transferCommandPool,
		&vertices, &indices, matToTex[mesh->mMaterialIndex]);
	return newMesh;
}


MeshModel::~MeshModel()
{
}
