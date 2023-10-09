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

	OctTree::OctTree(std::function<AABB(uint32_t)> getBox
		, std::function<OctNode*(uint32_t)> getNode
		, std::function<void(uint32_t, OctNode* node)> setNode
		, const AABB rootBox, int stopDepth)
{
	m_maxDepth = stopDepth;
	m_root.reset(nullptr);
	m_root = std::make_unique<OctNode>();
	m_root->box = rootBox;

	m_GetBoxFunction = getBox;
	m_GetNodeFunction= getNode;
	m_SetNodeFunction= setNode;
}

void OctTree::Insert(uint32_t entity)
{
	AABB obj = m_GetBoxFunction(entity);
	PerformInsert(m_root.get(), entity, obj);
}

void OctTree::TraverseRemove(uint32_t entity)
{
	bool result = PerformRemove(m_root.get(), entity);
	OO_ASSERT(result == true && "Tried to remove entity that does not exist in tree");
}

void OctTree::Remove(uint32_t entity)
{
	OctNode* node = m_GetNodeFunction(entity);
	OO_ASSERT(node && "Entity did not record the node");

	auto it = std::find(node->entities.begin(), node->entities.end(), entity);
	if (it != node->entities.end()) 
	{
		std::swap(*it, node->entities.back());
		node->entities.pop_back();
		m_SetNodeFunction(entity, nullptr);
		--m_nodes;
		return;
	}

	OO_ASSERT(false && "Entity does not exist in node it points to");
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
	GatherBoxWithDepth(m_root.get(), boxes, depth);

	return std::tuple< std::vector<AABB>, std::vector<uint32_t> >(boxes,depth);
}

std::tuple< std::vector<AABB>, std::vector<AABB> > OctTree::GetBoxesInFrustum(const Frustum& frust)
{
	std::vector<AABB> boxes;
	std::vector<AABB> testing;
	
	GatherFrustBoxes(m_root.get(), frust, boxes,testing);

	return std::tuple(boxes,testing);
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

void OctTree::GatherBoxWithDepth(OctNode* node, std::vector<AABB>& boxes, std::vector<uint32_t>& depth)
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
		GatherBoxWithDepth(node->children[i].get(), boxes, depth);
	}
}

void OctTree::GatherBox(OctNode* node, std::vector<AABB>& boxes)
{
	if (node == nullptr) return;

	boxes.push_back(node->box);
	for (size_t i = 0; i < s_num_children; i++)
	{
		GatherBox(node->children[i].get(), boxes);
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

void OctTree::GatherFrustBoxes(OctNode* node, const Frustum& frust, std::vector<AABB>& boxes, std::vector<AABB>& testingBox)
{	
	if (node == nullptr) return;

	oGFX::coll::Collision result = oGFX::coll::AABBInFrustum(frust, node->box);
	switch (result)
	{
	case oGFX::coll::INTERSECTS:
		// add to testing list
		testingBox.push_back(node->box);
		// check each child
		for (size_t i = 0; i < s_num_children; i++)
		{
			GatherFrustBoxes(node->children[i].get(), frust, boxes, testingBox);
		}
		break;
	case oGFX::coll::CONTAINS:
		// quickly gather all children
		GatherBox(node, boxes);
		break;
	case oGFX::coll::OUTSIDE:// no action needed
		break;
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
		m_SetNodeFunction(entity, node);
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
	m_SetNodeFunction(entity, node);
}

void OctTree::PerformClear(OctNode* node)
{
	if (node == nullptr) return;

	for (size_t i = 0; i < s_num_children; i++)
	{
		PerformClear(node->children[i].get());
	}

	for (size_t i = 0; i < node->entities.size(); i++)
	{
		m_SetNodeFunction(node->entities[i], nullptr);
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