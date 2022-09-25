#include "MeshModel.h"
#include "VulkanDevice.h"

#include <stdexcept>



void gfxModel::destroy(VkDevice device)
{

	auto DFS = [&](auto&& func, oGFX::BoneNode* pBoneNode) -> void
	{		
		if (pBoneNode == nullptr) return;
		// Recursion through all children nodes
		for (unsigned i = 0; i < pBoneNode->mChildren.size(); i++)
		{
			func(func, pBoneNode->mChildren[i]);
		}
		delete pBoneNode;
	};

	//DFS(DFS, m_boneNodes);
	//
	//for (auto& node : nodes)
	//{
	//	delete node;
	//	node = nullptr;
	//}
}

void gfxModel::loadNode(Node* parent,const aiScene* scene, const aiNode& node, uint32_t nodeIndex,
	ModelFileResource& cpuModel)
{
	Node* newNode = new Node();
	newNode->parent = parent;
	newNode->name = node.mName.C_Str();

	if (node.mNumChildren > 0)
	{
		for (size_t i = 0; i < node.mNumChildren; i++)
		{
			loadNode(newNode, scene, *node.mChildren[i], nodeIndex + static_cast<uint32_t>(i), cpuModel);
		}
	}

	if (node.mNumMeshes > 0)
	{
		for (size_t i = 0; i < node.mNumMeshes; i++)
		{
			aiMesh* aimesh = scene->mMeshes[node.mMeshes[i]];	
		}
	}

	if (parent == nullptr)
	{
		//nodes.push_back(newNode);	
	}
	else
	{
		parent->children.push_back(newNode);
	}
}

void offsetUpdateHelper(Node* parent, uint32_t& meshcount, uint32_t idxOffset, uint32_t vertOffset)
{
	//for (auto& node : parent->children)
	//{
	//	offsetUpdateHelper(node, meshcount, idxOffset, vertOffset);
	//}
	//for (auto& mesh :parent->meshes)
	//{
	//	mesh->indicesOffset += idxOffset;
	//	mesh->vertexOffset += vertOffset;
	//
	//	++meshcount;
	//}		
}

void gfxModel::updateOffsets(uint32_t idxOffset, uint32_t vertOffset)
{
	baseIndices += idxOffset;
	baseVertex += vertOffset;
	//for (auto& node : nodes)
	//{
	//	offsetUpdateHelper(node,this->meshCount, idxOffset, vertOffset);
	//}
}

oGFX::Mesh* gfxModel::processMesh(aiMesh* aimesh, const aiScene* scene, ModelFileResource& mData)
{
	oGFX::Mesh* mesh = new oGFX::Mesh;


	if (scene->HasAnimations() &&aimesh->HasBones())
	{
		for (size_t x = 0; x < aimesh->mNumBones; x++)
		{
			auto& bone = aimesh->mBones[x];
			std::string name(bone->mName.C_Str());

			auto iter = mData.strToBone.find(name);
			if (iter != mData.strToBone.end())
			{
				// Duplicate bone name!
				//assert(false);
			}
			else
			{
				//auto sz = mData.bones.size();
				//mData.bones.push_back({});
				//BoneOffset bo;
				//bo.transform = aiMat4_to_glm(bone->mOffsetMatrix);
				//mData.boneOffsets.emplace_back(bo);
				//mData.strToBone[name] = static_cast<uint32_t>(sz);
			}
			//cpuModel.boneWeights.resize()

			std::cout << "Bone :" << name << std::endl;
			std::cout << "Bone weights:" << bone->mNumWeights << std::endl;
			for (size_t y = 0; y < bone->mNumWeights; y++)
			{
				//std::cout << "\t v" << bone->mWeights[y].mVertexId << ":" << bone->mWeights[y].mWeight << std::endl;
			}
		}

	}

	uint32_t indicesCnt{ 0 };
	//indices.reserve(indices.size() + size_t(aimesh->mNumFaces) * aimesh->mFaces[0].mNumIndices);

	

	if (aimesh->mMaterialIndex >= 0)
	{
		mesh->textureIndex = aimesh->mMaterialIndex;
	}

	return mesh;
}

void gfxModel::loadNode(const aiScene* scene, const aiNode& node, Node* targetparent, ModelFileResource& cpuModel, glm::mat4 accMat)
{
	//glm::mat4 transform;
	//// if node has meshes, create a new scene object for it
	//if( node.mNumMeshes > 0)
	//{
	//	SceneObjekt newObject = new SceneObject;
	//	targetParent.addChild( newObject);
	//	// copy the meshes
	//	CopyMeshes( node, newObject);
	//	// the new object is the parent for all child nodes
	//	parent = newObject;
	//	transform.SetUnity();
	//} else
	//{
	//	// if no meshes, skip the node, but keep its transformation
	//	parent = targetparent;
	//	transform = aiMat4_to_glm(node.mTransformation) * accMat;
	//}
	//// continue for all child nodes
	//for( all node.mChildren)
	//	CopyNodesWithMeshes( node.mChildren[a], parent, transform);
}

void ModelFileResource::ModelSceneLoad(const aiScene* scene, 
	const aiNode& node,
	Node* parent,
	const glm::mat4 accMat)
{
	std::vector<Node*> curNodes;
	auto xform = aiMat4_to_glm(node.mTransformation) * accMat;
	Node* targetParent = parent;

	if (parent == nullptr)
	{
		sceneInfo = new Node();
		targetParent = sceneInfo;
		targetParent->name = "MdlSceneRoot";
	}
	if (node.mNumMeshes > 0)
	{
		this->sceneMeshCount += node.mNumMeshes;
		curNodes.resize(node.mNumMeshes);
		for (size_t i = 0; i < node.mNumMeshes; i++)
		{
			curNodes[i] = new Node();
			curNodes[i]->parent = targetParent;
			curNodes[i]->meshRef = node.mMeshes[i];
			curNodes[i]->name = node.mName.C_Str();
			curNodes[i]->transform = xform;
		}
		// setup nodes
		auto child = targetParent->children.insert(std::end(targetParent->children),std::begin(curNodes), std::end(curNodes));
		targetParent = *child;
	}		
	for (size_t i = 0; i < node.mNumChildren; i++)
	{
		ModelSceneLoad(scene, *node.mChildren[i], targetParent, xform);
	}
}

void ModelFileResource::ModelBoneLoad(const aiScene* scene, const aiNode& node, uint32_t vertOffset)
{	
	uint32_t sumVerts{};
	if (node.mNumMeshes > 0)
	{
		for (size_t i = 0; i < node.mNumMeshes; i++)
		{
			auto & aimesh = scene->mMeshes[node.mMeshes[i]];
			
			if (aimesh->HasBones())
			{
				for (size_t x = 0; x < aimesh->mNumBones; x++)
				{
					auto& bone = aimesh->mBones[x];
					std::string name(bone->mName.C_Str());

					size_t idx = 0;
					auto iter = strToBone.find(name);
					if (iter != strToBone.end())
					{
						// Duplicate bone name!
						idx = static_cast<size_t>(iter->second);
						//assert(false);
					}
					else
					{
						//idx = skeleton->m_boneNodes.size();
						//strToBone[name] = static_cast<uint32_t>(idx);
						//BoneOffset bo;
						//bo.transform = aiMat4_to_glm(bone->mOffsetMatrix);
						//boneOffsets.emplace_back(bo);
						//bones.push_back({});
					}

					std::cout << "Mesh :" << aimesh->mName.C_Str() << std::endl;
					std::cout << "Bone weights:" << bone->mNumWeights << std::endl;
					for (size_t y = 0; y < bone->mNumWeights; y++)
					{
						//auto& weight = bone->mWeights[y];
						//auto& vertWeight = boneWeights[weight.mVertexId];
						//auto& vertex = vertices[static_cast<size_t>(weight.mVertexId) + vertOffset + sumVerts];
						//
						//auto bNum = vertex.boneWeights++;
						////assert(bNum < 4); // CANNOT SUPPORT MORE THAN 4 BONES
						//if (bNum < 4)
						//{
						//	vertWeight.boneWeights[bNum] = weight.mWeight;
						//	vertWeight.boneIdx[bNum] = static_cast<uint32_t>(idx);
						//}
						//else
						//{
						//	std::cout << "Tried to load bones: " << bNum << std::endl;
						//}
						//std::cout << "\t v" << bone->mWeights[y].mVertexId << ":" << bone->mWeights[y].mWeight << std::endl;
					}
				}

			}			
			sumVerts += aimesh->mNumVertices;
		}
	}		
	for (size_t i = 0; i < node.mNumChildren; i++)
	{
		ModelBoneLoad(scene, *node.mChildren[i], vertOffset);
	}
}

ModelFileResource::~ModelFileResource()
{
	delete sceneInfo;
}	
	

	

