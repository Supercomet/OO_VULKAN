#pragma once

#include "MeshModel.h"

#include <string>
#include <map>

namespace simpleanim
{

struct alignas(__m128) VQS // From WR's CS460
{
public:
	using position_t = glm::vec3;
	using orientation_t = glm::quat;
	using scale_t = float;

	orientation_t q{ 1.0f,0.0f,0.0f,0.0f }; // Unit Quaternion
	position_t v{ 0.0f,0.0f,0.0f }; // Vector3, but treated as a pure quaternion! TAKE NOTE!
	scale_t s{ 1.0f };// Scalar, only supports uniform scaling

	VQS()
		: v{ 0.0f,0.0f,0.0f }, q{ 1.0f,0.0f,0.0f,0.0f }, s{ 1.0f }
	{}

	VQS(const position_t& pos, const orientation_t& quat, scale_t sca)
		: v{ pos }, q{ quat }, s{ sca }
	{}

	inline position_t GetPosition() const { return v; };
	inline void SetPosition(const position_t& pos) { v = pos; }

	inline orientation_t GetRotation() const { return q; }
	inline void SetRotation(const orientation_t& quat) { q = quat; }

	inline scale_t GetScale() const { return s; }
	inline void SetScale(float f) { s = f; }

	glm::mat4 ToMat4()
	{
		glm::mat4 res = glm::translate(glm::mat4{ 1.0f }, v) * glm::mat4_cast(q) * glm::scale(glm::mat4{ 1.0f }, glm::vec3{ s });
		return res;
	}

	inline VQS Inverse() const
	{
		VQS res;
		glm::quat qq = (1.0f / s) * (glm::inverse(q) * glm::quat{ 0.0f, -v.x, -v.y, -v.z } *q);
		res.v = { qq.x ,qq.y ,qq.z };
		res.q = glm::inverse(q);
		res.s = 1.0f / s;
		return res;
	}
};


using BoneIndex = uint32_t;

struct BoneNode
{
	std::string mName{ "BONE_NAME" };
	uint32_t m_BoneIndex;
	bool mbIsBoneNode{ false }; // Really a bone for skinning.
	std::weak_ptr<BoneNode> mpParent;
	std::vector<std::shared_ptr<BoneNode>> mChildren;

	VQS mModelSpaceLocalVqs{};	// Local transformation of the bone in model space
	VQS mModelSpaceGlobalVqs{};	// Global transformation of the bone in model space
};

class BoneInverseBindPoseInfo
{
public:
	int32_t mnBoneIndex{}; // Index of the bone
	VQS mBoneOffsetVQS; // The bone offset, aka inverse bind pose
};

constexpr uint32_t MAX_BONE_NUM = 4;
struct BoneWeights
{
	BoneWeights() :
		bone_idx{ 0, 0, 0, 0 },
		bone_weights{ 0.0f,0.0f,0.0f,0.0f }
	{}

	uint32_t bone_idx[MAX_BONE_NUM];
	float bone_weights[MAX_BONE_NUM];
};

struct SkinnedMesh
{
	std::map<std::string, BoneIndex> mBoneNameToBoneIndexMap; // Map of Bone Names to their index in the array of bones.
	std::vector<BoneInverseBindPoseInfo> mBoneInverseBindPoseBuffer; // Buffer for all bones with offset matrix in a contiguous array.
	std::vector<BoneWeights> mBoneWeightsBuffer;
	std::shared_ptr<BoneNode> mpRootNode{ nullptr };
};

struct LoadingConfig
{
	bool loadOnlyAnimation = false;
	bool animationNameOverride = false;
};

//--------------------------------------------------------------------------------
// Transform Point
// S(r) = [v, q, s]r = q(sr)q^-1 + v
// Where v is a PURE quaternion, q is a UNIT quaternion, s is a scalar
inline glm::vec3 operator*(const VQS& vqs, const glm::vec3& r)
{
	glm::quat v = glm::quat{ 0.0f, vqs.v.x, vqs.v.y, vqs.v.z };
	glm::quat res = vqs.q * (vqs.s * glm::quat{ 0.0f, r.x, r.y, r.z, }) * glm::inverse(vqs.q) + v;
	return glm::vec3{ res.x,res.y,res.z }; // Take the vector part of the quaternion.
}

//--------------------------------------------------------------------------------
// VQS Concatenation
// Convention is the same as glm matrix concatenation.
inline VQS operator*(const VQS& lhs, const VQS& rhs)
{
	VQS res;
	res.v = lhs * rhs.v;
	res.q = glm::normalize(lhs.q * rhs.q);
	res.s = lhs.s * rhs.s;
	return res;
}

bool LoadModelFromFile_Skeleton(const std::string& file, const LoadingConfig& config, ModelFileResource* model, SkinnedMesh* skinnedMesh);

void UpdateLocalToGlobalSpace(SkinnedMesh* skinnedMesh);

}
