/************************************************************************************//*!
\file           OctTree.cpp
\project        Ouroboros
\author         Jamie Kong, j.kong, 390004720 | code contribution (100%)
\par            email: j.kong\@digipen.edu
\date           Oct 02, 2022
\brief               Octtree for assignment

Copyright (C) 2022 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*//*************************************************************************************/
#include "OctTree.h"
#include "BoudingVolume.h"
#include "Collision.h"
#include <algorithm>
#include <iostream>

namespace oGFX {

	OctTree::OctTree(std::function<AABB(uint32_t)> getBox, const AABB rootBox, int stopDepth)
{
	m_maxDepth = stopDepth;
	m_root.reset(nullptr);
	m_root = std::make_unique<OctNode>();
	m_root->box = rootBox;

	m_GetBoxFunction = getBox;
}

void OctTree::Insert(uint32_t entity)
{
	AABB obj = m_GetBoxFunction(entity);
	PerformInsert(m_root.get(), entity, obj);
}

void OctTree::Remove(uint32_t entity)
{
	bool result = PerformRemove(m_root.get(), entity);
	OO_ASSERT(result == true && "Tried to remove entity that does not exist in tree");
}

std::vector<uint32_t> OctTree::GetEntitiesInFrustum(const Frustum& frust)
{
	std::vector<uint32_t> entities;
	std::vector<uint32_t> depth;
	GatherEntities(m_root.get(), entities, depth);
	return entities;
}

std::tuple<std::vector<AABB>, std::vector<uint32_t>> OctTree::GetActiveBoxList()
{
	std::vector<AABB> boxes; std::vector<uint32_t> depth;

	boxes.push_back(m_root->box);
	depth.push_back(0);
	GatherBox(m_root.get(), boxes, depth);

	return std::tuple< std::vector<AABB>, std::vector<uint32_t> >(boxes,depth);
}

void OctTree::ClearTree()
{
	PerformClear(m_root.get());

	m_entities.clear();
}

uint32_t OctTree::size() const
{
	return m_nodes;
}

void OctTree::GatherBox(OctNode* node, std::vector<AABB>& boxes, std::vector<uint32_t>& depth)
{
	if (node == nullptr) return;

	//if (node->type == OctNode::LEAF)
	if (node->entities.size())
	{
		boxes.push_back(node->box);
		depth.push_back(node->depth);
	}
	for (size_t i = 0; i < s_num_children; i++)
	{
		GatherBox(node->children[i].get(), boxes, depth);
	}
}

void OctTree::GatherEntities(OctNode* node, std::vector<uint32_t>& entities, std::vector<uint32_t>& depth)
{
	if (node == nullptr) return;

	if (node->type == OctNode::LEAF)
	{
		for (size_t i = 0; i < node->entities.size(); i++)
		{
			entities.push_back(node->entities[i]);
			depth.push_back(node->depth);
		}		
	}
	for (size_t i = 0; i < s_num_children; i++)
	{
		GatherEntities(node->children[i].get(), entities, depth);
	}
}

void OctTree::SplitNode(OctNode* node)
{
	const uint32_t currDepth = node->depth + 1;

	Point3D position{};
	float step = node->box.halfExt.x * 0.5f;
	assert(step != 0.0f);
	for (size_t i = 0; i < s_num_children; i++)
	{
		position.x = ((i & 1) ? step : -step);
		position.y = ((i & 2) ? step : -step);
		position.z = ((i & 4) ? step : -step);
		AABB childBox;
		childBox.center = node->box.center + position;
		childBox.halfExt = Point3D{ step,step,step };
		node->children[i] = std::make_unique<OctNode>();
		node->children[i]->depth = currDepth;
		node->children[i]->box = childBox;
	}

	// distribute entities
	std::vector<uint32_t> entCpy = std::move(node->entities);
	for (uint32_t e : entCpy)
	{
		AABB entBox = m_GetBoxFunction(e);
		PerformInsert(node, e, entBox);
	}
}

void OctTree::PerformInsert(OctNode* node, uint32_t entity, const AABB& objBox)
{
	OO_ASSERT(node);

	const uint32_t currDepth = node->depth + 1;
	// we reached max depth, turn into leaf
	if (currDepth > m_maxDepth)
	{
		node->type = OctNode::LEAF;
		node->nodeID = ++m_nodes;
		node->entities.push_back(entity);
		return;
	}

	if (node->children[0] == nullptr) 
	{
		SplitNode(node);
	}
	for (size_t i = 0; i < s_num_children; i++)
	{
		if (oGFX::coll::AabbContains(node->children[i]->box, objBox))
		{
			PerformInsert(node->children[i].get(), entity, objBox);			
			return;
		}
	}

	// not contained in any child
	node->nodeID = ++m_nodes;
	node->entities.push_back(entity);	
}

void OctTree::PerformClear(OctNode* node)
{
	if (node == nullptr) return;

	for (size_t i = 0; i < s_num_children; i++)
	{
		PerformClear(node->children[i].get());
	}

	node->entities.clear();
}

bool OctTree::PerformRemove(OctNode* node, uint32_t entity)
{
	if (node == nullptr) return false;

	auto it = std::find(node->entities.begin(), node->entities.end(), entity);

	// Check if the element was found
	if (it != node->entities.end()) {
		// Element found, erase it from the vector
		node->entities.erase(it);
		--m_nodes;
		return true;
	}
	else {
		// Element not found
		// search children
		for (size_t i = 0; i < s_num_children; i++)
		{
			if (PerformRemove(node->children[i].get(), entity))
			{
				return true;
			}
		}
	}

	return false;
}

}// end namespace oGFX