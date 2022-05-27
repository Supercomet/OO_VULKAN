#include "Collision.h"
#include <algorithm>

namespace coll
{

bool PointSphere(const Point3D& p, const Sphere& s)
{
	glm::vec3 d = (glm::vec3)p - (glm::vec3)s.centre;
	float r_sq = (s.radius * s.radius);
	return (glm::dot(d, d) <= r_sq);
}

bool PointSphere(const Sphere& s, const Point3D& p)
{
	return PointSphere(p,s);
}

bool SphereSphere(const Sphere& a, const Sphere& b)
{
	return PointSphere(b.centre,
		Sphere{a.centre, a.radius + b.radius});
}

bool AabbAabb(const AABB& a, const AABB& b)

{
	// for each axis { X, Y, Z }
	const auto aMax = a.max();
	const auto aMin = a.min();

	const auto bMax = b.max();
	const auto bMin = b.min();

	// todo optimize for XYZ based on game
	for(unsigned int i = 0; i < 3; ++i)
	{
		// if no overlap for the axis, no overlap overall
		if(aMax[i] < bMin[i] || bMax[i] < aMin[i])
			return false;
	}
	return true;
}

bool SphereAabb(const Sphere& s, const AABB& a)
{
	float sqrDist = SqDistPointAabb(s.centre, a);

	return sqrDist <= s.radius*s.radius;
}

bool SphereAabb(const AABB& a, const Sphere& s)
{
	return SphereAabb(s,a);
}

bool PointAabb(const Point3D& p, const AABB& aabb)
{
	const auto min = aabb.min();
	const auto max = aabb.max();

	for (uint32_t i = 0; i < 3; i++)
	{
		if (p.pos[i] < min[i] || p.pos[i] > max[i])
			return false;
	}
	return true;
}

bool PointAabb(const AABB& aabb, const Point3D& p)
{
	return PointAabb(p,aabb);
}

bool PointPlane(const Point3D& q, const Plane& p)
{
	
	return PointPlane(q,p,EPSILON);
}

bool PointPlane(const Plane& p, const Point3D& q)
{
	return PointPlane(q,p);
}

bool PointPlane(const Point3D& q, const Plane& p, float epsilon)
{
	float d = glm::dot(q.pos, glm::vec3(p.normal));
	return std::abs(d- p.normal.w) < epsilon;
}

float SqDistPointAabb(const Point3D& p, const AABB& a)
{
	float sqrDist = 0.0f;

	glm::vec3 min = a.min();
	glm::vec3 max = a.max();

	// for each axis
	for (glm::vec3::length_type i = 0; i < 3; i++)
	{
		float v = p.pos[i];
		if (v < min[i]) sqrDist += (min[i] - v) * (min[i] - v);
		if (v > max[i]) sqrDist += (v- max[i]) * (v - max[i]);
	}

	return sqrDist;
}

float DistPointPlane(const Point3D& q, const Plane& p)
{
	auto n = glm::vec3(p.normal);// can skip dot if normalized
	return (glm::dot(n,q.pos)-p.normal.w)/ glm::dot(n,n);
}

float ScalarTriple(const Point3D& u, const Point3D& v, const Point3D& w)
{
	return glm::dot(glm::cross(u.pos, v.pos), w.pos);
}

bool BaryCentricTriangle(const Point3D& p1, const Point3D& p2, const Point3D& p3, float u, float v, float w)
{
	return false;
}

//bool BaryCentricTriangle(const Point3D& p, const Triangle& tri, float& u, float& v, float& w)
//{
//	// Check if p.pos in vertex region outside tri.v1.pos
//	glm::vec3 ab = tri.v2.pos - tri.v1.pos;
//	glm::vec3 ac = tri.v3.pos - tri.v1.pos;
//	glm::vec3 ap = p.pos - tri.v1.pos;
//	float d1 = glm::dot(ab, ap);
//	float d2 = glm::dot(ac, ap);
//	if (d1 <= 0.0f && d2 <= 0.0f) return tri.v1.pos; // barycentric coordinates (1,0,0)
//											// Check if p.pos in vertex region outside tri.v2.pos
//	glm::vec3 bp = p.pos - tri.v2.pos;
//	float d3 = glm::dot(ab, bp);
//	float d4 = glm::dot(ac, bp);
//	if (d3 >= 0.0f && d4 <= d3) return tri.v2.pos; // barycentric coordinates (0,1,0)
//										  // Check if p.pos in edge region of AB, if so return projection of p.pos onto AB
//	float vc = d1 * d4 - d3 * d2;
//	if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
//	{
//		float v = d1 / (d1 - d3);
//		return true; // barycentric coordinates (1-v,v,0)
//	}
//	// Check if p.pos in vertex region outside C
//	glm::vec3 cp = p.pos - c;
//	float d5 = glm::dot(ab, cp);
//	float d6 = glm::dot(ac, cp);
//	if (d6 >= 0.0f && d5 <= d6) return true; // barycentric coordinates (0,0,1)
//
//		// Check if p.pos in edge region of AC, if so return projection of p.pos onto AC
//	float vb = d5 * d2 - d1 * d6;
//	if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
//	{
//		float w = d2 / (d2 - d6);
//		return true; // barycentric coordinates (1-w,0,w)
//	}
//	// Check if p.pos in edge region of BC, if so return projection of p.pos onto BC
//	float va = d3 * d6 - d5 * d4;
//	if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
//	{
//		float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
//		return tri.v2.pos + w * (c - tri.v2.pos); // barycentric coordinates (0,1-w,w)
//	}
//	// p.pos inside face region. Compute Q through its barycentric coordinates (u,v,w)
//	float denom = 1.0f / (va + vb + vc);
//	float v = vb * denom;
//	float w = vc * denom;
//	return tri.v1.pos + ab * v + ac * w; // = u*tri.v1.pos + v*tri.v2.pos + w*c, u = va * denom = 1.0f-v-w
//
//}

bool RayPlane(const Ray& r, const Plane& p, float& t, Point3D& pt)
{
	glm::vec3 pNorm = p.normal;
	float d = p.normal.w;

	float divs = glm::dot(pNorm, r.direction);
	if (std::abs(divs) > EPSILON)
	{
		t = -(glm::dot(pNorm, r.start.pos) + d) / divs;
		if (t >= EPSILON)
		{
			return true;
		}
	}
	return false;
}

bool RayPlane(const Ray& r, const Plane& p, float& t)
{
	Point3D pt;
	return RayPlane(r, p, t, pt);
}

bool RayPlane(const Ray& r, const Plane& p)
{
	float t;
	Point3D pt;
	return RayPlane(r, p, t, pt);
}

bool RayPlane(const Plane& p, const Ray& r)
{
	return RayPlane(r,p);
}

bool RayAabb(const Ray& r, const AABB& a)
{
	float tmin;
	float tmax;
	Point3D p;
	return RayAabb(r, a, tmin, tmax, p);	
}

bool RayAabb(const Ray& r, const AABB& a, float& tmin)
{
	float tmax;
	Point3D p;
	return RayAabb(r, a, tmin, tmax, p);
}

bool RayAabb(const Ray& r, const AABB& a, float& tmin, float& tmax, Point3D& p)
{
	tmin = 0.0f; // set to -FLT_MAX to get first hit on line
	tmax = FLT_MAX; // set to max distance ray can travel (for segment)
						  // For all three slabs
	
	auto aMin = a.min();
	auto aMax = a.max();

	for (int i = 0; i < 3; i++)
	{
		if (std::abs(r.direction[i]) < EPSILON)
		{
			// Ray is parallel to slab. No hit if origin not within slab
			if (r.start.pos[i] < aMin[i] || r.start.pos[i] > aMax[i]) return 0;
		}
		else
		{
			// Compute intersection t value of ray with near and far plane of slab
			float ood = 1.0f / r.direction[i];
			float t1 = (aMin[i] - r.start.pos[i]) * ood;
			float t2 = (aMax[i] - r.start.pos[i]) * ood;
			// Make t1 be intersection with near plane, t2 with far plane
			if (t1 > t2) std::swap(t1, t2);
			// Compute the intersection of slab intersection intervals
			if (t1 > tmin) tmin = t1;
			if (t2 < tmax) tmax = t2;
			// Exit with no collision as soon as slab intersection becomes empty
			if (tmin > tmax) return 0;
		}
	}
	// Ray intersects all 3 slabs. Return point (q) and intersection t value (tmin)
	p= r.start.pos + r.direction * tmin;
	return true;
}

bool RaySphere(const Ray& r, const Sphere& s)
{
	float t{ FLT_MAX };
	return RaySphere(r, s, t);
}

bool RaySphere(const Ray& r, const Sphere& s, float& t)
{
	return false;
}

bool RayTriangle(const Ray& r, const Triangle& tri)
{
	float t{ FLT_MAX };
	return RayTriangle(r, tri, t);
}

bool RayTriangle(const Ray& r, const Triangle& tri, float& t)
{
	Plane plane = { glm::cross(tri.v1.pos, tri.v2.pos), tri.v1.pos };	
	if (RayPlane(r, plane, t))
	{
		return PointInTriangle(r.start.pos + r.direction * t, tri);
	}
	return false;
}

bool PointInTriangle(const Point3D& p, const Point3D& v1, const Point3D& v2, const Point3D& v3)
{
	// Translate point to origin
	Point3D a{ v1 - p };
	Point3D b{ v2 - p };
	Point3D c{ v3 - p };

	// Legrange's identity
	float ab = glm::dot(a.pos, b.pos);
	float ac = glm::dot(a.pos, c.pos);
	float bc = glm::dot(b.pos, c.pos);
	float cc = glm::dot(c.pos, c.pos);

	// normals pab and pbc point in the same direction
	if (bc * ac - cc * ab < 0.0f) return 0;

	float bb = glm::dot(b.pos, b.pos);
	// normals for pab and pca point in the same direction
	if (ab * bc - ac * bb < 0.0f) return 0;

	// p must be in the triangle
	return 1;

}

bool PointInTriangle(const Point3D& p, const Triangle& t)
{
	return PointInTriangle(p,t.v1,t.v2,t.v3);
}

bool PointInTriangle(const Triangle& t, const Point3D& p)
{
	return PointInTriangle(p,t.v1,t.v2,t.v3);
}

bool PlaneSphere(const Plane& p, const Sphere& s)
{
	float dist = glm::dot(s.centre.pos, glm::vec3(p.normal))-p.normal.w;
	return std::abs(dist) <= s.radius;
}

bool PlaneAabb(const Plane& p, const AABB& a)
{
	Point3D c = a.center;
	const glm::vec3& e = a.halfExt;

	float r = e[0] * std::abs(p.normal[0]) 
			+ e[1] * std::abs(p.normal[1]) 
			+ e[2] * std::abs(p.normal[2]);

	float s = glm::dot(glm::vec3{ p.normal }, a.center);

	return std::abs(s) <= r;
}

}// end namespace coll