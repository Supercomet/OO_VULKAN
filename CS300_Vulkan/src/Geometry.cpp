#include "Geometry.h"

Point3D::Point3D(const glm::vec3& v)
	:pos{v}
{
}

Point3D::operator const glm::vec3& ()const
{
	return this->pos;
}

Point3D::operator glm::vec3& ()
{
	return this->pos;
}

Point3D Point3D::operator-(const Point3D& rhs) const
{
	return Point3D(this->pos - rhs.pos);
}

AABB::AABB() :
	center{},
	halfExt{ 0.5f,0.5f,0.5f }
{
}

AABB::AABB(glm::vec3 min, glm::vec3 max)
{
	auto mid = (min + max) / 2.0f;
	center = mid;
	halfExt = { max - mid };
}

glm::vec3 AABB::max()const
{
	return this->center + this->halfExt;
}

glm::vec3 AABB::min()const
{
	return this->center - this->halfExt;
}

Sphere::Sphere() : centre{}, radius{ 1.0f }
{
}

Sphere::Sphere(Point3D p, float r) :
	centre{ p },
	radius{ r }
{
}

Sphere::Sphere(glm::vec3 p, float r) :
	centre{ p },
	radius{ r }
{
}

Ray::Ray() :
	start{},
	direction{ 1.0 }
{
}

Ray::Ray(glm::vec3 s, glm::vec3 dir) :
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

Plane::Plane(glm::vec3 n, glm::vec3 p)
{
	n = glm::normalize(n);
	normal = { n, (glm::dot(p,n)) };
}

std::pair<glm::vec3, glm::vec3> Plane::ToPointNormal() const
{
	return std::pair<glm::vec3, glm::vec3>(glm::vec3{ normal }, glm::vec3{ 0.0,0.0,normal.w/normal.z });
}

Triangle::Triangle():
v1{glm::vec3{0.5f,0.5f,0.5f}},
v2{glm::vec3{0.0f,0.5f,0.5f}},
v3{glm::vec3{0.5f,0.0f,0.5f}}
{
}

Triangle::Triangle(glm::vec3 a, glm::vec3 b, glm::vec3 c)
	:v1{a},
	v2{b},
	v3{c}
{
}
