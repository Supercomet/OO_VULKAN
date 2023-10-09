/************************************************************************************//*!
\file           OctTree.h
\project        Ouroboros
\author         Jamie Kong, j.kong, 390004720 | code contribution (100%)
\par            email: j.kong\@digipen.edu
\date           Oct 02, 2022
\brief              Octtree for assignment

Copyright (C) 2022 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*//*************************************************************************************/
#include "MathCommon.h"
#include "Geometry.h"
#include "VulkanUtils.h"

#include <vector>
#include <tuple>
#include <functional>
#include <memory>

namespace oGFX {

struct OctNode;

class OctTree
{
public:
	inline static constexpr uint32_t s_num_children = 8;
	inline static constexpr uint32_t s_stop_depth = 8;
public:
	OctTree(std::function<AABB(uint32_t)> getBox
		, std::function<OctNode* (uint32_t)> getNode
		, std::function<void(uint32_t, OctNode* node)> setNode
		, const AABB rootBox = { Point3D{-500.0f},Point3D{500.0f} } ,int stopDepth = s_stop_depth);

	void Insert(uint32_t entity);
	void TraverseRemove(uint32_t entity);
	void Remove(uint32_t entity);

	std::vector<uint32_t> GetEntitiesInFrustum(const Frustum& frust);
	std::tuple<std::vector<AABB>,std::vector<uint32_t> > GetActiveBoxList();
	std::tuple< std::vector<AABB>,std::vector<AABB> > GetBoxesInFrustum(const Frustum& frust);

	void ClearTree();
	uint32_t size() const;

private:
	std::function<AABB(uint32_t)> m_GetBoxFunction;
	std::function<OctNode*(uint32_t)> m_GetNodeFunction;
	std::function<void(uint32_t, OctNode*)> m_SetNodeFunction;
	std::unique_ptr<OctNode> m_root{};

	uint32_t m_entitiesCnt{};
	std::vector<uint32_t> m_entities;
	uint32_t m_maxDepth{ s_stop_depth };

	uint32_t m_nodes{};
	uint32_t m_boxesInsertCnt[s_num_children];

	void GatherBoxWithDepth(OctNode* node,std::vector<AABB>& boxes, std::vector<uint32_t>& depth);
	void GatherBox(OctNode* node,std::vector<AABB>& boxes);
	void GatherEntities(OctNode* node,std::vector<uint32_t>& entities, std::vector<uint32_t>& depth);
	void GatherFrustBoxes(OctNode* node, const Frustum& frust,std::vector<AABB>& boxes, std::vector<AABB>& testingBox);

	void PerformInsert(OctNode* node, uint32_t entity, const AABB& objBox);
	bool PerformRemove(OctNode* node, uint32_t entity);
	void SplitNode(OctNode* node);
	void PerformClear(OctNode* node);

};

struct OctNode
{
	enum Type
	{
		INTERNAL,
		LEAF,
	}type{ Type::INTERNAL };

	AABB box{};
	uint32_t depth{};
	uint32_t nodeID{};

	std::vector<uint32_t> entities;
	std::unique_ptr<OctNode> children[OctTree::s_num_children];
};

}// end namespace oGFX