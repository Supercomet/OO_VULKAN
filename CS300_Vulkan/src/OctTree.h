#include "MathCommon.h"
#include "Geometry.h"

#include <vector>
#include <tuple>
#include <memory>

static constexpr uint32_t s_num_children = 8;
static constexpr uint32_t s_stop_depth = 1000;
static constexpr uint32_t s_stop_triangles = 30;

struct OctNode
{
	enum Type
	{
		INTERNAL,
		LEAF,
	}type;

	AABB box;
	uint32_t depth{};
	uint32_t nodeID{};

	std::vector<Point3D> vertices;
	std::vector<uint32_t> indices;
	std::unique_ptr<OctNode> children[s_num_children];
};

class OctTree
{
public:
	OctTree(const std::vector<Point3D>& vertices, const std::vector<uint32_t>& indices);

	std::tuple< std::vector<Point3D>, std::vector<uint32_t>,std::vector<uint32_t> > GetTriangleList();
private:
	std::unique_ptr<OctNode> m_root{};
	uint32_t m_trianglesSaved{};
	uint32_t m_nodes{};

	void SplitNode(OctNode* node, AABB box,const std::vector<Point3D>& vertices, const std::vector<uint32_t>& indices);
	void SplitTrianglesAlongPlane(const std::vector<Point3D>& vertices, const std::vector<uint32_t>& indices,const Plane& plane,
		std::vector<Point3D>& positiveVerts, std::vector<uint32_t>& positiveIndices,
		std::vector<Point3D>& negativeVerts, std::vector<uint32_t>& negativeIndices
	);

	void GatherTriangles(OctNode* node,std::vector<Point3D>& vertices, std::vector<uint32_t>& indices,std::vector<uint32_t>& depth);

};
