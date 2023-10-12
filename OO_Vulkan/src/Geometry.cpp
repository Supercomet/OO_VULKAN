/************************************************************************************//*!
\file           Geometry.cpp
\project        Ouroboros
\author         Jamie Kong, j.kong, 390004720 | code contribution (100%)
\par            email: j.kong\@digipen.edu
\date           Oct 02, 2022
\brief              For assignment geometry

Copyright (C) 2022 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*//*************************************************************************************/
#include "Geometry.h"

namespace oGFX {
AABB::AABB() :
	center{},
	halfExt{ 0.5f,0.5f,0.5f }
{
}

AABB::AABB(const Point3D& min, const Point3D& max)
{
	auto mid = (min + max) / 2.0f;
	center = mid;
	halfExt = { max - mid };
}

Point3D AABB::max()const
{
	return this->center + this->halfExt;
}

Point3D AABB::min()const
{
	return this->center - this->halfExt;
}

AABB2D::AABB2D(glm::vec2 _min, glm::vec2 _max):
	min{_min},
	max{_max}
{
}

Sphere::Sphere() : center{}, radius{ 1.0f }
{
}

Sphere::Sphere(Point3D p, float r) :
	center{ p },
	radius{ r }
{
}


Ray::Ray() :
	start{},
	direction{ 1.0 }
{
}

Ray::Ray(const Point3D& s, const Point3D& dir) :
	start{ s },
	direction{ dir }
{
}

Plane::Plane() 
{
	static const glm::vec3 norm = []() {
		glm::vec3 n{1.0f};
		n = glm::normalize(n);
		return n;
	}();
	normal = { norm,0.0f };
}

Plane::Plane(const Point3D& n, const Point3D& p)
{
	auto nv = glm::normalize(n);
	normal = { nv, (glm::dot(p,nv)) };
}

Plane::Plane(const Point3D& n, float d)
	:
	normal{ glm::normalize(n),d }
{
}

std::pair<glm::vec3, glm::vec3> Plane::ToPointNormal() const
{
	float d = normal.w / normal.z;
	assert(d != 0.0f);
	return std::pair<glm::vec3, glm::vec3>( glm::vec3{ 0.0,0.0, d},glm::vec3{ normal });
}

Triangle::Triangle():
v0{glm::vec3{0.5f,0.5f,0.5f}},
v1{glm::vec3{0.0f,0.5f,0.5f}},
v2{glm::vec3{0.5f,0.0f,0.5f}}
{
}

Triangle::Triangle(const Point3D& a, const Point3D& b, const Point3D& c)
	:v0{a},
	v1{b},
	v2{c}
{
}

Frustum Frustum::CreateFromViewProj(glm::mat4 m)
{
	oGFX::Frustum f;

	// Right clipping plane
	f.right.normal.x = m[0][3] + m[0][0];
	f.right.normal.y = m[1][3] + m[1][0];
	f.right.normal.z = m[2][3] + m[2][0];
	f.right.normal.w = m[3][3] + m[3][0];

	// Left clipping plane
	f.left.normal.x = m[0][3] - m[0][0];
	f.left.normal.y = m[1][3] - m[1][0];
	f.left.normal.z = m[2][3] - m[2][0];
	f.left.normal.w = m[3][3] - m[3][0];

	// Top clipping plane
	f.top.normal.x = m[0][3] - m[0][1];
	f.top.normal.y = m[1][3] - m[1][1];
	f.top.normal.z = m[2][3] - m[2][1];
	f.top.normal.w = m[3][3] - m[3][1];

	// Bottom clipping plane
	f.bottom.normal.x = m[0][3] + m[0][1];
	f.bottom.normal.y = m[1][3] + m[1][1];
	f.bottom.normal.z = m[2][3] + m[2][1];
	f.bottom.normal.w = m[3][3] + m[3][1];

	// Far clipping plane
	f.planeFar.normal.x = m[0][2];
	f.planeFar.normal.y = m[1][2];
	f.planeFar.normal.z = m[2][2];
	f.planeFar.normal.w = m[3][2];

	// Near clipping plane
	f.planeNear.normal.x = m[0][3] - m[0][2];
	f.planeNear.normal.y = m[1][3] - m[1][2];
	f.planeNear.normal.z = m[2][3] - m[2][2];
	f.planeNear.normal.w = m[3][3] - m[3][2];

	return f;
}

}// end namespace oGFX
