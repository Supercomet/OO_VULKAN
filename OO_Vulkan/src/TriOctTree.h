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

#include <vector>
#include <tuple>
#include <memory>

namespace oGFX {

struct TriOctNode;

class TriOctTree
{
public:
	inline static constexpr uint32_t s_num_children = 8;
	inline static constexpr uint32_t s_stop_depth = 8;
	inline static constexpr uint32_t s_stop_triangles = 50;
public:
	TriOctTree(const std::vector<Point3D>& vertices, const std::vector<uint32_t>& indices, int maxTrangles = s_stop_triangles);

	std::tuple< std::vector<Point3D>, std::vector<uint32_t>,std::vector<uint32_t> > GetTriangleList();
	std::tuple< std::vector<AABB>,std::vector<uint32_t> > GetActiveBoxList();

	void Rebuild();
	void SetTriangles(int maxTrianges);
	int GetTriangles();
	float progress();

	uint32_t size() const;

private:
	std::unique_ptr<TriOctNode> m_root{};
	uint32_t m_trianglesSaved{};
	uint32_t m_trianglesRemaining{};
	uint32_t m_nodes{};
	uint32_t m_TrianglesSliced{};
	std::vector<Point3D> m_vertices; std::vector<uint32_t> m_indices;
	uint32_t m_maxNodesTriangles{ s_stop_triangles };
	uint32_t m_maxDepth{ s_stop_depth };

	uint32_t m_boxesInsertCnt[s_num_children];

	void SplitNode(TriOctNode* node,const AABB& box,const std::vector<Point3D>& vertices, const std::vector<uint32_t>& indices);
	void PartitionTrianglesAlongPlane(const std::vector<Point3D>& vertices, const std::vector<uint32_t>& indices,const Plane& plane,
		std::vector<Point3D>& positiveVerts, std::vector<uint32_t>& positiveIndices,
		std::vector<Point3D>& negativeVerts, std::vector<uint32_t>& negativeIndices
	);


	void GatherTriangles(TriOctNode* node,std::vector<Point3D>& vertices, std::vector<uint32_t>& indices,std::vector<uint32_t>& depth);
	void GatherBox(TriOctNode* node,std::vector<AABB>& boxes, std::vector<uint32_t>& depth);
};

struct TriOctNode
{
	enum Type
	{
		INTERNAL,
		LEAF,
	}type{ Type::INTERNAL };

	AABB box{};
	uint32_t depth{};
	uint32_t nodeID{};

	std::vector<Point3D> vertices;
	std::vector<uint32_t> indices;
	std::unique_ptr<TriOctNode> children[TriOctTree::s_num_children];
};

}// end namespace oGFX