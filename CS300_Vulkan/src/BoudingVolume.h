#pragma once
#include <vector>

#include "Geometry.h"

namespace oGFX::BV
{
	static constexpr size_t RITTERS_METHOD{ 1 };
	enum EPOS : size_t
	{		
		_6 =  1,
		_14 = 2,
		_26 = 3,
		_98 = 6,
	};

	void MostSeparatedPointsOnAABB(uint32_t& min, uint32_t& max, const std::vector<Point3D>& points);
	void MostSeperatedPointsOnAxes(uint32_t& min, uint32_t& max, const std::vector<Point3D>& points, size_t range);

	void ExtremePointsAlongDirection(const glm::vec3& axis, const std::vector<Point3D>& points, uint32_t& min, uint32_t& max, float* min_val = nullptr, float* max_val = nullptr);

	void ExpandSphereAboutPoint(Sphere& s, const Point3D& point, size_t range);
	void SphereFromDistantPoints(Sphere& s, const std::vector<Point3D>& points);

	Sphere HorizonDisk(const Point3D& view, const Sphere& s);

	void BoundingAABB(AABB& aabb, const std::vector<Point3D>& points);

	void LarsonSphere(Sphere& s, const std::vector<Point3D>& points, size_t range = 2);

	void RitterSphere(Sphere& s, const std::vector<Point3D>& points);

	void CovarianceMatrix(Mat3& cov, const std::vector<Point3D>& points);
	void SymSchur2(const Mat3& a, int32_t p, int32_t q, float& c, float& s);
	void Jacobi(Mat3& cov, Mat3& v);
	void EigenSphere(Sphere& s, const std::vector<Point3D>& points);

	void RittersEigenSphere(Sphere& s, const std::vector<Point3D>& points);

};

