#include "BspTree.h"
#include "BoudingVolume.h"
#include <algorithm>
#include <iostream>

BspTree::BspTree(const std::vector<Point3D>& vertices, const std::vector<uint32_t>& indices, int maxTriangles)
{
	m_vertices = vertices;
	m_indices = indices;
	m_maxNodesTriangles = maxTriangles;

	Rebuild();
}


std::tuple< std::vector<Point3D>, std::vector<uint32_t>,std::vector<uint32_t> > BspTree::GetTriangleList()
{
	std::vector<Point3D> vertices;
	std::vector<uint32_t> indices;
	std::vector<uint32_t> depth;

	GatherTriangles(m_root.get(), vertices, indices,depth);

	return std::tuple< std::vector<Point3D>, std::vector<uint32_t>,std::vector<uint32_t> >(vertices,indices,depth);
}


void BspTree::Rebuild()
{
	for (size_t i = 0; i < s_num_children; i++) m_planePartitionCount[i] = 0;

	m_root.reset(nullptr);
	m_root = std::make_unique<BspNode>();

	SplitNode(m_root.get(), m_root->m_splitPlane, m_vertices, m_indices);

	for (size_t i = 0; i < s_num_children; i++)
		std::cout << "Inserted into plane [" << (i?"positive" :"negative") << "] - " << m_planePartitionCount[i] << " times\n";
}

void BspTree::SetTriangles(int maxTrianges)
{
	m_maxNodesTriangles = maxTrianges;
	m_maxNodesTriangles = std::max(static_cast<uint32_t>(m_maxNodesTriangles) , s_stop_triangles);
}

int BspTree::GetTriangles()
{
	return m_maxNodesTriangles;
}

uint32_t BspTree::size() const
{
	return m_nodes;
}

void BspTree::SplitNode(BspNode* node, const Plane& box, const std::vector<Point3D>& vertices, const std::vector<uint32_t>& indices)
{
	const uint32_t currDepth = node->depth + 1;
	const uint32_t numTriangles = static_cast<uint32_t>(indices.size()) / 3;

	if (currDepth > m_maxDepth
		|| (numTriangles <= m_maxNodesTriangles))
	{
		node->type = BspNode::LEAF;
		node->nodeID = ++m_nodes;

		m_trianglesSaved += numTriangles;
		node->vertices =(vertices);
		node->indices = (indices);
	}
	else
	{
		node->type = BspNode::INTERNAL;

		// Get a nice splitting plane
		Plane splittingPlane = PickSplittingPlane(vertices, indices);
		node->m_splitPlane = splittingPlane;

		// planesplit
		std::vector<Point3D> splitVerts[s_num_children];
		std::vector<uint32_t> splitIndices[s_num_children];
		PartitionTrianglesAlongPlane(vertices, indices, splittingPlane,splitVerts[0],splitIndices[0],splitVerts[1],splitIndices[1]);

		// Split and recurse
		for (size_t i = 0; i < s_num_children; i++)
		{
			if(splitVerts->size())
				m_planePartitionCount[i] += splitVerts->size();

			node->children[i] = std::make_unique<BspNode>();
			node->children[i]->depth = currDepth;

			SplitNode(node->children[i].get(),splittingPlane, splitVerts[i], splitIndices[i]);
		}
	}

}

void BspTree::PartitionTrianglesAlongPlane(const std::vector<Point3D>& vertices, const std::vector<uint32_t>& indices, const Plane& plane,
	std::vector<Point3D>& positiveVerts, std::vector<uint32_t>& positiveIndices,
	std::vector<Point3D>& negativeVerts, std::vector<uint32_t>& negativeIndices)
{
	const uint32_t numTriangles = static_cast<uint32_t>(indices.size()) / 3;

	for (size_t i = 0; i < numTriangles; i++)
	{
		Point3D v[3];
		v[0] = vertices[indices[i*3 + 0]];
		v[1] = vertices[indices[i*3 + 1]];
		v[2] = vertices[indices[i*3 + 2]];

		Triangle t(v[0], v[1], v[2]);

		auto orientation = oGFX::BV::ClassifyTriangleToPlane(t, plane);

		switch (orientation)
		{
		case TriangleOrientation::COPLANAR:
		case TriangleOrientation::POSITIVE:
		{
			auto index = static_cast<uint32_t>(positiveVerts.size());
			for (uint32_t i = 0; i < 3; i++)
			{
				positiveVerts.push_back(v[i]);
				positiveIndices.push_back(index + i);
			}
		}
		break;
		case TriangleOrientation::NEGATIVE:
		{
			auto index = static_cast<uint32_t>(negativeVerts.size());
			for (uint32_t i = 0; i < 3; i++)
			{
				negativeVerts.push_back(v[i]);
				negativeIndices.push_back(index + i);
			}
		}
		break;
		case TriangleOrientation::STRADDLE:
		{
			++m_TrianglesSliced;
			oGFX::BV::SliceTriangleAgainstPlane(t, plane,
				positiveVerts, positiveIndices,
				negativeVerts,negativeIndices);
		}
		break;
		}

	}
}

Plane BspTree::PickSplittingPlane(const std::vector<Point3D>& vertices, const std::vector<uint32_t>& indices)
{
	Plane bestPlane;

	// Blend factor for optimizing for balance or splits (should be tweaked)
	const float K = 0.8f;
	// Variables for tracking best splitting plane seen so far
	float bestScore = FLT_MAX;
	// Try the plane of each polygon as a dividing plane
	size_t numTriangles = indices.size() / 3;
	for (int i = 0; i < numTriangles; i++) {
		int numInFront = 0, numBehind = 0, numStraddling = 0;

		Point3D v1[3];
		v1[0] = vertices[indices[i*3 + 0]];
		v1[1] = vertices[indices[i*3 + 1]];
		v1[2] = vertices[indices[i*3 + 2]];
		Triangle t1(v1[0], v1[1], v1[2]);

		Plane plane = oGFX::BV::PlaneFromTriangle(t1);
		// Test against all other polygons
		for (int j = 0; j < numTriangles; j++) {
			// Ignore testing against self
			if (i == j) continue;

			Point3D v2[3];
			v2[0] = vertices[indices[j*3 + 0]];
			v2[1] = vertices[indices[j*3 + 1]];
			v2[2] = vertices[indices[j*3 + 2]];
			Triangle t2(v2[0], v2[1], v2[2]);
			// Keep standing count of the various poly-plane relationships
			switch (oGFX::BV::ClassifyTriangleToPlane(t2, plane)) 
			{
			case TriangleOrientation::COPLANAR:
			/* Coplanar polygons treated as being in front of plane */
			case TriangleOrientation::POSITIVE:
			numInFront++;
			break;
			case TriangleOrientation::NEGATIVE:
			numBehind++;
			break;
			case TriangleOrientation::STRADDLE:
			numStraddling++;
			break;
			}
		}
		// Compute score as a weighted combination (based on K, with K in range
		// 0..1) between balance and splits (lower score is better)
		float score=K* numStraddling + (1.0f - K) * std::abs(numInFront - numBehind);
		if (score < bestScore) {
			bestScore = score;
			bestPlane = plane;
		}
	}
	return bestPlane;
}

void BspTree::GatherTriangles(BspNode* node, std::vector<Point3D>& outVertices, std::vector<uint32_t>& outIndices,std::vector<uint32_t>& depth)
{
	if (node == nullptr) return;

	if (node->type == BspNode::LEAF)
	{
		const auto triCnt = node->indices.size() / 3;
		for (size_t i = 0; i < triCnt; i++)
		{
			const auto anchor = static_cast<uint32_t>(outVertices.size());
			depth.push_back(node->nodeID);

			Point3D v[3];
			v[0] = node->vertices[node->indices[i*3 + 0]];
			v[1] = node->vertices[node->indices[i*3 + 1]];
			v[2] = node->vertices[node->indices[i*3 + 2]];

			for (uint32_t j = 0; j < 3; j++)
			{
				outVertices.push_back(v[j]);
				outIndices.push_back(anchor + j);
			}
		}
	}
	else
	{
		for (size_t i = 0; i < s_num_children; i++)
		{
			if (node->children[i])
			{
				GatherTriangles(node->children[i].get(),outVertices,outIndices,depth);
			}
		}
	}	

}
