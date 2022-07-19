#include "MathCommon.h"
#include "Geometry.h"

#include <vector>
#include <tuple>
#include <memory>

struct BspNode;

class BspTree
{
public:
	inline static constexpr uint32_t s_num_children = 2;
	inline static constexpr uint32_t s_stop_depth = 8;
	inline static constexpr uint32_t s_stop_triangles = 1000;
public:
	BspTree(const std::vector<Point3D>& vertices, const std::vector<uint32_t>& indices, int maxTrangles = s_stop_triangles);

	std::tuple< std::vector<Point3D>, std::vector<uint32_t>,std::vector<uint32_t> > GetTriangleList();

	void Rebuild();
	void SetTriangles(int maxTrianges);
	int GetTriangles();

	uint32_t size() const;

private:
	std::unique_ptr<BspNode> m_root{};
	uint32_t m_trianglesSaved{};
	uint32_t m_nodes{};
	uint32_t m_TrianglesSliced{};
	std::vector<Point3D> m_vertices; std::vector<uint32_t> m_indices;
	uint32_t m_maxNodesTriangles{ s_stop_triangles };
	uint32_t m_maxDepth{ s_stop_depth };

	uint32_t m_planePartitionCount[s_num_children];

	void SplitNode(BspNode* node,const Plane& plane,const std::vector<Point3D>& vertices, const std::vector<uint32_t>& indices);
	void PartitionTrianglesAlongPlane(const std::vector<Point3D>& vertices, const std::vector<uint32_t>& indices,const Plane& plane,
		std::vector<Point3D>& positiveVerts, std::vector<uint32_t>& positiveIndices,
		std::vector<Point3D>& negativeVerts, std::vector<uint32_t>& negativeIndices
	);

	Plane PickSplittingPlane(const std::vector<Point3D>& vertices, const std::vector<uint32_t>& indices);

	void GatherTriangles(BspNode* node,std::vector<Point3D>& vertices, std::vector<uint32_t>& indices,std::vector<uint32_t>& depth);

};

struct BspNode
{
	enum Type
	{
		INTERNAL,
		LEAF,
	}type{ Type::INTERNAL };

	Plane m_splitPlane{};

	uint32_t depth{};
	uint32_t nodeID{};

	std::vector<Point3D> vertices;
	std::vector<uint32_t> indices;
	std::unique_ptr<BspNode> children[BspTree::s_num_children];
};