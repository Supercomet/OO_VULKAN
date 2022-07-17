#pragma once

#include "MathCommon.h"
#include <tuple>

using Point3D = glm::vec3;
using Mat3 = glm::mat3;
constexpr static float EPSILON = { 0.001f };

struct Plane
{
	Plane();
	Plane(const Point3D& n, const Point3D& p);
	Plane(const Point3D& n, float d);
	glm::vec4 normal;
	std::pair<glm::vec3, glm::vec3> ToPointNormal() const;
};

struct Triangle
{
	Triangle();
	Triangle(const Point3D& a, const Point3D& b, const Point3D& c);
	Point3D v0;
	Point3D v1;
	Point3D v2;
};

struct Sphere
{
	Sphere();
	Sphere(Point3D p, float r);
	Point3D center;
	float radius;
};

struct AABB
{
	AABB();
	AABB(const Point3D& min,const Point3D& max);
	Point3D center;
	Point3D halfExt{0.5f,0.5f,0.5f};

	Point3D max() const;
	Point3D min() const;
};

struct Ray
{
	Ray();
	Ray(const Point3D& s,const Point3D& dir);
	Point3D start;
	glm::vec3 direction;
};
