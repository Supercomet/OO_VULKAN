#include "SimpleAnim.h"

#pragma warning( push )
#pragma warning( disable : 26451 ) // vendor overflow
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#pragma warning( pop )

namespace simpleanim
{

inline glm::vec3 assimp_to_glm(const aiVector3D& value)
{
	return glm::vec3{ value.x, value.y, value.z };
}

// glm is column major, assimp is row major
inline glm::mat4 assimp_to_glm(const aiMatrix4x4& value)
{
	glm::mat4 res;
	//{ m[0][0], m[0][1], m[0][2], m[0][3] };
	//{ m[1][0], m[1][1], m[1][2], m[1][3] };
	//{ m[2][0], m[2][1], m[2][2], m[2][3] };
	//{ m[3][0], m[3][1], m[3][2], m[3][3] };
	res[0] = { value.a1, value.b1, value.c1, value.d1 };
	res[1] = { value.a2, value.b2, value.c2, value.d2 };
	res[2] = { value.a3, value.b3, value.c3, value.d3 };
	res[3] = { value.a4, value.b4, value.c4, value.d4 };

	return res;
}

// Forward declarations
void LoadFromAssimp_BoneAndWeights(aiMesh* paiMesh, SkinnedMesh* mesh, std::vector<BoneWeights>& vertexBuffer);
void BuildMySkeletonRecursive(SkinnedMesh* mesh, aiNode* ainode, std::shared_ptr<BoneNode> node);

bool LoadModelFromFile_Skeleton(const std::string& file, const LoadingConfig& config, Model* model, SkinnedMesh* skinnedMesh)
{
	auto flags = 0;
	flags |= aiProcess_Triangulate;
	flags |= aiProcess_GenSmoothNormals;
	flags |= aiProcess_ImproveCacheLocality;
	flags |= aiProcess_CalcTangentSpace;
	
	Assimp::Importer import;
	const aiScene* pAiScene = import.ReadFile(file, flags);

	// Check if the assimp scene is valid
	if (pAiScene == nullptr || pAiScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || pAiScene->mRootNode == nullptr)
	{
		std::cout << "[Assimp] Assimp::Importer Error ! " << import.GetErrorString() << std::endl;
		return false;
	}

	// The size must match the vertex buffer.
	skinnedMesh->mBoneWeightsBuffer.resize(model->vertices.size());
	skinnedMesh->mpRootNode = std::make_shared<BoneNode>();

	for (unsigned mesh_i = 0; mesh_i < pAiScene->mNumMeshes; ++mesh_i)
	{
		aiMesh* paiMesh = pAiScene->mMeshes[mesh_i];
		LoadFromAssimp_BoneAndWeights(paiMesh, skinnedMesh, skinnedMesh->mBoneWeightsBuffer);
	}

	{
		BuildMySkeletonRecursive(skinnedMesh, pAiScene->mRootNode, skinnedMesh->mpRootNode);
	}

	return true;
}

void BuildMySkeletonRecursive(SkinnedMesh* mesh, aiNode* ainode, std::shared_ptr<BoneNode> node)
{
	std::string node_name{ ainode->mName.data };
	glm::mat4x4 node_transform = assimp_to_glm(ainode->mTransformation);

	std::string cName = node_name.substr(node_name.find_last_of("_") + 1);

	// Save the bone index
	bool bIsBoneNode = false;
	auto iter = mesh->mBoneNameToBoneIndexMap.find(node_name);
	if (iter != mesh->mBoneNameToBoneIndexMap.end())
	{
		bIsBoneNode = true;
		node->m_BoneIndex = iter->second;
	}

	// Since Assimp only stores the final transformation,
	// we will need to decompose the transformation matrix.
	// Also, to convert into our VQS structure.
	aiVector3D pos, scale;
	aiQuaternion qua;
	ainode->mTransformation.Decompose(scale, qua, pos);

	if ((scale.x - scale.y) > 0.0001f || (scale.x - scale.z) > 0.0001f)
	{
		static bool firstTime = true;
		if (firstTime)
		{
			// Non-uniform scale bone detected...
			__debugbreak();
			firstTime = false;
		}
	}

	// Copy information from assimp to our nodes.
	node->mName = node_name;
	node->mbIsBoneNode = bIsBoneNode;
	node->mModelSpaceLocalVqs = VQS{ assimp_to_glm(pos) , glm::quat{ qua.w,qua.x,qua.y,qua.z } , scale.x };
	node->mChildren.reserve(ainode->mNumChildren);

	// Recursion through all children
	for (unsigned i = 0; i < ainode->mNumChildren; i++)
	{
		node->mChildren.push_back(std::make_shared<BoneNode>()); // Create the child node.
		BuildMySkeletonRecursive(mesh, ainode->mChildren[i], node->mChildren[i]);
		node->mChildren[i]->mpParent = node; // Link the child to the parent node.
	}
}

void LoadFromAssimp_BoneAndWeights(aiMesh* paiMesh, SkinnedMesh* mesh, std::vector<BoneWeights>& vertexBuffer)
{
	bool firstWarningForWeights = true;
	uint32_t mnNumBones = 0;

	for (unsigned i_bone = 0; i_bone < paiMesh->mNumBones; ++i_bone)
	{
		BoneIndex bone_index = 0;
		std::string bone_name = paiMesh->mBones[i_bone]->mName.C_Str();

		// Find whether the bone aleady exists
		if (mesh->mBoneNameToBoneIndexMap.find(bone_name) == mesh->mBoneNameToBoneIndexMap.end())
		{
			// Allocate index for new bone
			bone_index = mnNumBones++;
			BoneInverseBindPoseInfo& invBindPoseInfo = mesh->mBoneInverseBindPoseBuffer.emplace_back(BoneInverseBindPoseInfo{});

			// Map the name of this bone to this index. (map<string,int>)
			mesh->mBoneNameToBoneIndexMap[bone_name] = bone_index;

			// Assimp stores the transformation as matrix.
			// We decompose and convert into our storage method... (Either our matrix or VQS)
			aiVector3D pos, scale;
			aiQuaternion qua;
			paiMesh->mBones[i_bone]->mOffsetMatrix.Decompose(scale, qua, pos);

			// Setup information
			invBindPoseInfo.mnBoneIndex = bone_index;
			invBindPoseInfo.mBoneOffsetVQS = VQS{ assimp_to_glm(pos) , glm::quat{ qua.w,qua.x,qua.y,qua.z }, scale.x };
		}
		else
		{
			// Somehow there is a duplicate name or it already exist!
			// Just retrieve the index from the map
			bone_index = mesh->mBoneNameToBoneIndexMap[bone_name];
		}

		// Add the bone weights for the vertices for the current bone
		for (unsigned j = 0; j < paiMesh->mBones[i_bone]->mNumWeights; ++j)
		{
			const unsigned vertexID = paiMesh->mBones[i_bone]->mWeights[j].mVertexId;
			const float weight = paiMesh->mBones[i_bone]->mWeights[j].mWeight;

			bool success = false;

			for (int slot = 0; slot < 4; ++slot)
			{
				auto& vertex = vertexBuffer[vertexID];

				if (vertex.bone_weights[slot] == 0.0f)
				{
					vertex.bone_idx[slot] = bone_index;
					vertex.bone_weights[slot] = weight;
					success = true;
					break;
				}
			}

			// Check if the number of weights is >4, just in case, since we dont support
			if (!success && firstWarningForWeights)
			{
				// Vertex already has 4 bone weights assigned.
				//assert(false && "Bone weights >4 is not supported.");
				//__debugbreak();
				firstWarningForWeights = false;
			}
		}
	}//end for
}

void UpdateLocalToGlobalSpace(SkinnedMesh* skinnedMesh)
{
	if (skinnedMesh == nullptr)
		return;

	BoneNode* pBoneNode = skinnedMesh->mpRootNode.get();

	auto DFS = [&](auto&& func, BoneNode* pBoneNode, const VQS& parentTransform) -> void
	{
		const VQS node_local_transform = pBoneNode->mModelSpaceLocalVqs;
		const VQS node_global_transform = parentTransform * node_local_transform;
		pBoneNode->mModelSpaceGlobalVqs = node_global_transform; // Update global space

		// If the node isn't a bone then we don't care.
		if (pBoneNode->mbIsBoneNode && pBoneNode->m_BoneIndex >= 0)
		{
			//outputBoneModelSpaceBuffer[pBoneNode->m_BoneIndex] = pBoneNode->GetModelSpaceGlobalVqs();
		}

		// Recursion through all children nodes, passing in the current global transform.
		for (unsigned i = 0; i < pBoneNode->mChildren.size(); i++)
		{
			func(func, pBoneNode->mChildren[i].get(), node_global_transform);
		}
	};

	DFS(DFS, pBoneNode, VQS{});
}

}