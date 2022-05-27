#pragma once
#include "glm/glm.hpp"
#include <tuple>

struct Point3D
{
	Point3D() = default;
	Point3D(const glm::vec3& v);

	glm::vec3 pos;

	operator const glm::vec3& ()const;
	operator glm::vec3& ();
	Point3D operator-(const Point3D& rhs) const;
};

struct Plane
{
	Plane();
	Plane(glm::vec3 n, glm::vec3 p);
	glm::vec4 normal;
	std::pair<glm::vec3, glm::vec3> ToPointNormal() const;
};

struct Triangle
{
	Triangle();
	Triangle(glm::vec3 a, glm::vec3 b, glm::vec3 c);
	Point3D v1;
	Point3D v2;
	Point3D v3;
};

struct Sphere
{
	Sphere();
	Sphere(Point3D p, float r);
	Sphere(glm::vec3 p, float r);
	Point3D centre;
	float radius;
};

struct AABB
{
	AABB();
	AABB(glm::vec3 min,glm::vec3 max);
	glm::vec3 center;
	glm::vec3 halfExt;

	glm::vec3 max() const;
	glm::vec3 min() const;
};

struct Ray
{
	Ray();
	Ray(glm::vec3 s, glm::vec3 dir);
	Point3D start;
	glm::vec3 direction;
};
