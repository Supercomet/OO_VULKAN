#pragma once
#include "geometry.h"
#include "collision.h"
#include <glm/glm.hpp>
#include <string>

using stdstring = std::string;
using Vector3 = glm::vec3;

using Point = Point3D;
using Sphere = Sphere;
using Aabb = AABB;
using Plane = Plane;
using Ray = Ray;
using Triangle = Triangle;

int RunAllTests();

#pragma region Declarations
void PrintTestHeader(const stdstring& testName);

void SphereSphereTest1(const stdstring& testName);
void SphereSphereTest2(const stdstring& testName);
void SphereSphereTest3(const stdstring& testName);
void SphereSphereTest4(const stdstring& testName);
void SphereSphereTest5(const stdstring& testName);
void SphereSphereTest6(const stdstring& testName);
void SphereSphereTest7(const stdstring& testName);
void SphereSphereTest8(const stdstring& testName);
void SphereSphereTest9(const stdstring& testName);
void SphereSphereTest10(const stdstring& testName);
void SphereSphereTest11(const stdstring& testName);
void SphereSphereTest12(const stdstring& testName);
void SphereSphereTest13(const stdstring& testName);
void SphereSphereTest14(const stdstring& testName);
void SphereSphereTest15(const stdstring& testName);

void AabbAabbTest1(const stdstring& testName);
void AabbAabbTest2(const stdstring& testName);
void AabbAabbTest3(const stdstring& testName);
void AabbAabbTest4(const stdstring& testName);
void AabbAabbTest5(const stdstring& testName);
void AabbAabbTest6(const stdstring& testName);
void AabbAabbTest7(const stdstring& testName);
void AabbAabbTest8(const stdstring& testName);
void AabbAabbTest9(const stdstring& testName);
void AabbAabbTest10(const stdstring& testName);
void AabbAabbTest11(const stdstring& testName);
void AabbAabbTest12(const stdstring& testName);
void AabbAabbTest13(const stdstring& testName);
void AabbAabbTest14(const stdstring& testName);
void AabbAabbTest15(const stdstring& testName);
void AabbAabbTest16(const stdstring& testName);
void AabbAabbTest17(const stdstring& testName);
void AabbAabbTest18(const stdstring& testName);
void AabbAabbTest19(const stdstring& testName);
void AabbAabbTest20(const stdstring& testName);
void AabbAabbTest21(const stdstring& testName);
void AabbAabbTest22(const stdstring& testName);
void AabbAabbTest23(const stdstring& testName);
void AabbAabbTest24(const stdstring& testName);
void AabbAabbTest25(const stdstring& testName);
void AabbAabbTest26(const stdstring& testName);
void AabbAabbTest27(const stdstring& testName);
void AabbAabbTest28(const stdstring& testName);
void AabbAabbTest29(const stdstring& testName);
void AabbAabbTest30(const stdstring& testName);
void AabbAabbTest31(const stdstring& testName);
void AabbAabbTest32(const stdstring& testName);
void AabbAabbTest33(const stdstring& testName);
void AabbAabbTest34(const stdstring& testName);
void AabbAabbTest35(const stdstring& testName);
void AabbAabbTest36(const stdstring& testName);
void AabbAabbTest37(const stdstring& testName);
void AabbAabbTest38(const stdstring& testName);
void AabbAabbTest39(const stdstring& testName);
void AabbAabbTest40(const stdstring& testName);
void AabbAabbTest41(const stdstring& testName);
void AabbAabbTest42(const stdstring& testName);
void AabbAabbTest43(const stdstring& testName);
void AabbAabbTest44(const stdstring& testName);
void AabbAabbTest45(const stdstring& testName);
void AabbAabbTest46(const stdstring& testName);
void AabbAabbTest47(const stdstring& testName);
void AabbAabbTest48(const stdstring& testName);
void AabbAabbTest49(const stdstring& testName);
void AabbAabbTest50(const stdstring& testName);
void AabbAabbTest51(const stdstring& testName);
void AabbAabbTest52(const stdstring& testName);
void AabbAabbTest53(const stdstring& testName);
void AabbAabbTest54(const stdstring& testName);
void AabbAabbTest55(const stdstring& testName);
void AabbAabbTest56(const stdstring& testName);
void AabbAabbTest57(const stdstring& testName);
void AabbAabbTest58(const stdstring& testName);
void AabbAabbTest59(const stdstring& testName);
void AabbAabbTest60(const stdstring& testName);
void AabbAabbTest61(const stdstring& testName);
void AabbAabbTest62(const stdstring& testName);
void AabbAabbTest63(const stdstring& testName);
void AabbAabbTest64(const stdstring& testName);
void AabbAabbTest65(const stdstring& testName);
void AabbAabbTest66(const stdstring& testName);
void AabbAabbTest67(const stdstring& testName);
void AabbAabbTest68(const stdstring& testName);
void AabbAabbTest69(const stdstring& testName);
void AabbAabbTest70(const stdstring& testName);
void AabbAabbTest71(const stdstring& testName);
void AabbAabbTest72(const stdstring& testName);
void AabbAabbTest73(const stdstring& testName);
void AabbAabbTest74(const stdstring& testName);
void AabbAabbTest75(const stdstring& testName);
void AabbAabbTest76(const stdstring& testName);
void AabbAabbTest77(const stdstring& testName);
void AabbAabbTest78(const stdstring& testName);
void AabbAabbTest79(const stdstring& testName);
void AabbAabbTest80(const stdstring& testName);
void AabbAabbTest81(const stdstring& testName);
void AabbAabbTest82(const stdstring& testName);
void AabbAabbTest83(const stdstring& testName);
void AabbAabbTest84(const stdstring& testName);

void PointPlaneTest1(const stdstring& testName);
void PointPlaneTest2(const stdstring& testName);
void PointPlaneTest3(const stdstring& testName);
void PointPlaneTest4(const stdstring& testName);
void PointPlaneTest5(const stdstring& testName);
void PointPlaneTest6(const stdstring& testName);
void PointPlaneTest7(const stdstring& testName);
void PointPlaneTest8(const stdstring& testName);
void PointPlaneTest9(const stdstring& testName);
void PointPlaneTest10(const stdstring& testName);
void PointPlaneTest11(const stdstring& testName);
void PointPlaneTest12(const stdstring& testName);
void PointPlaneTest13(const stdstring& testName);
void PointPlaneTest14(const stdstring& testName);
void PointPlaneTest15(const stdstring& testName);
void PointPlaneTest16(const stdstring& testName);
void PointPlaneTest17(const stdstring& testName);
void PointPlaneTest18(const stdstring& testName);
void PointPlaneTest19(const stdstring& testName);
void PointPlaneTest20(const stdstring& testName);
void PointPlaneTest21(const stdstring& testName);
void PointPlaneTest22(const stdstring& testName);
void PointPlaneTest23(const stdstring& testName);
void PointPlaneTest24(const stdstring& testName);
void PointPlaneTest25(const stdstring& testName);
void PointPlaneTest26(const stdstring& testName);
void PointPlaneTest27(const stdstring& testName);
void PointPlaneTest28(const stdstring& testName);
void PointPlaneTest29(const stdstring& testName);
void PointPlaneTest30(const stdstring& testName);
void PointPlaneTest31(const stdstring& testName);
void PointPlaneTest32(const stdstring& testName);
void PointPlaneTest33(const stdstring& testName);
void PointPlaneTest34(const stdstring& testName);
void PointPlaneTest35(const stdstring& testName);
void PointPlaneTest36(const stdstring& testName);
void PointPlaneTest37(const stdstring& testName);
void PointPlaneTest38(const stdstring& testName);
void PointPlaneTest39(const stdstring& testName);
void PointPlaneTest40(const stdstring& testName);

void PointSphereTest1(const stdstring& testName);
void PointSphereTest2(const stdstring& testName);
void PointSphereTest3(const stdstring& testName);
void PointSphereTest4(const stdstring& testName);
void PointSphereTest5(const stdstring& testName);
void PointSphereTest6(const stdstring& testName);
void PointSphereTest7(const stdstring& testName);
void PointSphereTest8(const stdstring& testName);
void PointSphereTest9(const stdstring& testName);
void PointSphereTest10(const stdstring& testName);
void PointSphereTest11(const stdstring& testName);
void PointSphereTest12(const stdstring& testName);
void PointSphereTest13(const stdstring& testName);
void PointSphereTest14(const stdstring& testName);
void PointSphereTest15(const stdstring& testName);
void PointSphereTest16(const stdstring& testName);
void PointSphereTest17(const stdstring& testName);
void PointSphereTest18(const stdstring& testName);
void PointSphereTest19(const stdstring& testName);
void PointSphereTest20(const stdstring& testName);
void PointSphereTest21(const stdstring& testName);
void PointSphereTest22(const stdstring& testName);
void PointSphereTest23(const stdstring& testName);
void PointSphereTest24(const stdstring& testName);
void PointSphereTest25(const stdstring& testName);
void PointSphereTest26(const stdstring& testName);

void PointAabbTest1(const stdstring& testName);
void PointAabbTest2(const stdstring& testName);
void PointAabbTest3(const stdstring& testName);
void PointAabbTest4(const stdstring& testName);
void PointAabbTest5(const stdstring& testName);
void PointAabbTest6(const stdstring& testName);
void PointAabbTest7(const stdstring& testName);
void PointAabbTest8(const stdstring& testName);
void PointAabbTest9(const stdstring& testName);
void PointAabbTest10(const stdstring& testName);
void PointAabbTest11(const stdstring& testName);
void PointAabbTest12(const stdstring& testName);
void PointAabbTest13(const stdstring& testName);
void PointAabbTest14(const stdstring& testName);
void PointAabbTest15(const stdstring& testName);
void PointAabbTest16(const stdstring& testName);
void PointAabbTest17(const stdstring& testName);
void PointAabbTest18(const stdstring& testName);
void PointAabbTest19(const stdstring& testName);
void PointAabbTest20(const stdstring& testName);
void PointAabbTest21(const stdstring& testName);
void PointAabbTest22(const stdstring& testName);
void PointAabbTest23(const stdstring& testName);
void PointAabbTest24(const stdstring& testName);
void PointAabbTest25(const stdstring& testName);
void PointAabbTest26(const stdstring& testName);
void PointAabbTest27(const stdstring& testName);
void PointAabbTest28(const stdstring& testName);
void PointAabbTest29(const stdstring& testName);
void PointAabbTest30(const stdstring& testName);
void PointAabbTest31(const stdstring& testName);
void PointAabbTest32(const stdstring& testName);
void PointAabbTest33(const stdstring& testName);
void PointAabbTest34(const stdstring& testName);
void PointAabbTest35(const stdstring& testName);
void PointAabbTest36(const stdstring& testName);
void PointAabbTest37(const stdstring& testName);
void PointAabbTest38(const stdstring& testName);
void PointAabbTest39(const stdstring& testName);
void PointAabbTest40(const stdstring& testName);
void PointAabbTest41(const stdstring& testName);
void PointAabbTest42(const stdstring& testName);
void PointAabbTest43(const stdstring& testName);
void PointAabbTest44(const stdstring& testName);
void PointAabbTest45(const stdstring& testName);
void PointAabbTest46(const stdstring& testName);
void PointAabbTest47(const stdstring& testName);
void PointAabbTest48(const stdstring& testName);
void PointAabbTest49(const stdstring& testName);
void PointAabbTest50(const stdstring& testName);
void PointAabbTest51(const stdstring& testName);
void PointAabbTest52(const stdstring& testName);
void PointAabbTest53(const stdstring& testName);
void PointAabbTest54(const stdstring& testName);
void PointAabbTest55(const stdstring& testName);
void PointAabbTest56(const stdstring& testName);
void PointAabbTest57(const stdstring& testName);
void PointAabbTest58(const stdstring& testName);
void PointAabbTest59(const stdstring& testName);
void PointAabbTest60(const stdstring& testName);
void PointAabbTest61(const stdstring& testName);
void PointAabbTest62(const stdstring& testName);
void PointAabbTest63(const stdstring& testName);
void PointAabbTest64(const stdstring& testName);
void PointAabbTest65(const stdstring& testName);
void PointAabbTest66(const stdstring& testName);
void PointAabbTest67(const stdstring& testName);
void PointAabbTest68(const stdstring& testName);
void PointAabbTest69(const stdstring& testName);
void PointAabbTest70(const stdstring& testName);
void PointAabbTest71(const stdstring& testName);
void PointAabbTest72(const stdstring& testName);
void PointAabbTest73(const stdstring& testName);
void PointAabbTest74(const stdstring& testName);
void PointAabbTest75(const stdstring& testName);
void PointAabbTest76(const stdstring& testName);
void PointAabbTest77(const stdstring& testName);
void PointAabbTest78(const stdstring& testName);
void PointAabbTest79(const stdstring& testName);
void PointAabbTest80(const stdstring& testName);
void PointAabbTest81(const stdstring& testName);
void PointAabbTest82(const stdstring& testName);
void PointAabbTest83(const stdstring& testName);
void PointAabbTest84(const stdstring& testName);

void BarycentricTriangleTest0(const stdstring& testName);
void BarycentricTriangleTest1(const stdstring& testName);
void BarycentricTriangleTest2(const stdstring& testName);
void BarycentricTriangleTest3(const stdstring& testName);
void BarycentricTriangleTest4(const stdstring& testName);
void BarycentricTriangleTest5(const stdstring& testName);
void BarycentricTriangleTest6(const stdstring& testName);
void BarycentricTriangleTest7(const stdstring& testName);
void BarycentricTriangleTest8(const stdstring& testName);
void BarycentricTriangleTest9(const stdstring& testName);
void BarycentricTriangleTest10(const stdstring& testName);
void BarycentricTriangleTest11(const stdstring& testName);
void BarycentricTriangleTest12(const stdstring& testName);
void BarycentricTriangleTest13(const stdstring& testName);
void BarycentricTriangleTest14(const stdstring& testName);
void BarycentricTriangleTest15(const stdstring& testName);
void BarycentricTriangleTest16(const stdstring& testName);
void BarycentricTriangleTest17(const stdstring& testName);
void BarycentricTriangleTest18(const stdstring& testName);
void BarycentricTriangleTest19(const stdstring& testName);
void BarycentricTriangleTest20(const stdstring& testName);
void BarycentricTriangleTest21(const stdstring& testName);
void BarycentricTriangleTest22(const stdstring& testName);
void BarycentricTriangleTest23(const stdstring& testName);
void BarycentricTriangleTest24(const stdstring& testName);
void BarycentricTriangleTest25(const stdstring& testName);
void BarycentricTriangleTest26(const stdstring& testName);
void BarycentricTriangleTest27(const stdstring& testName);
void BarycentricTriangleTest28(const stdstring& testName);
void BarycentricTriangleTest29(const stdstring& testName);
void BarycentricTriangleTest30(const stdstring& testName);
void BarycentricTriangleTest31(const stdstring& testName);
void BarycentricTriangleTest32(const stdstring& testName);
void BarycentricTriangleTest33(const stdstring& testName);
void BarycentricTriangleTest34(const stdstring& testName);
void BarycentricTriangleTest35(const stdstring& testName);
void BarycentricTriangleTest36(const stdstring& testName);
void BarycentricTriangleTest37(const stdstring& testName);
void BarycentricTriangleTest38(const stdstring& testName);
void BarycentricTriangleTest39(const stdstring& testName);
void BarycentricTriangleTest40(const stdstring& testName);
void BarycentricTriangleTest41(const stdstring& testName);
void BarycentricTriangleTest42(const stdstring& testName);
void BarycentricTriangleTest43(const stdstring& testName);
void BarycentricTriangleTest44(const stdstring& testName);
void BarycentricTriangleTest45(const stdstring& testName);
void BarycentricTriangleTest46(const stdstring& testName);
void BarycentricTriangleTest47(const stdstring& testName);
void BarycentricTriangleTest48(const stdstring& testName);
void BarycentricTriangleTest49(const stdstring& testName);
void BarycentricTriangleTest50(const stdstring& testName);
void BarycentricTriangleTest51(const stdstring& testName);
void BarycentricTriangleTest52(const stdstring& testName);
void BarycentricTriangleTest53(const stdstring& testName);
void BarycentricTriangleTest54(const stdstring& testName);
void BarycentricTriangleTest55(const stdstring& testName);
void BarycentricTriangleTest56(const stdstring& testName);
void BarycentricTriangleTest57(const stdstring& testName);
void BarycentricTriangleTest58(const stdstring& testName);
void BarycentricTriangleTest59(const stdstring& testName);
void BarycentricTriangleTest60(const stdstring& testName);
void BarycentricTriangleTest61(const stdstring& testName);
void BarycentricTriangleTest62(const stdstring& testName);
void BarycentricTriangleTest63(const stdstring& testName);
void BarycentricTriangleTest64(const stdstring& testName);

void RayPlaneTest1(const stdstring& testName);
void RayPlaneTest2(const stdstring& testName);
void RayPlaneTest3(const stdstring& testName);
void RayPlaneTest4(const stdstring& testName);
void RayPlaneTest5(const stdstring& testName);
void RayPlaneTest6(const stdstring& testName);
void RayPlaneTest7(const stdstring& testName);
void RayPlaneTest8(const stdstring& testName);
void RayPlaneTest9(const stdstring& testName);
void RayPlaneTest10(const stdstring& testName);
void RayPlaneTest11(const stdstring& testName);
void RayPlaneTest12(const stdstring& testName);
void RayPlaneTest13(const stdstring& testName);
void RayPlaneTest14(const stdstring& testName);
void RayPlaneTest15(const stdstring& testName);
void RayPlaneTest16(const stdstring& testName);
void RayPlaneTest17(const stdstring& testName);
void RayPlaneTest18(const stdstring& testName);
void RayPlaneTest19(const stdstring& testName);
void RayPlaneTest20(const stdstring& testName);
void RayPlaneTest21(const stdstring& testName);
void RayPlaneTest22(const stdstring& testName);
void RayPlaneTest23(const stdstring& testName);
void RayPlaneTest24(const stdstring& testName);
void RayPlaneTest25(const stdstring& testName);
void RayPlaneTest26(const stdstring& testName);
void RayPlaneTest27(const stdstring& testName);
void RayPlaneTest28(const stdstring& testName);
void RayPlaneTest29(const stdstring& testName);
void RayPlaneTest30(const stdstring& testName);
void RayPlaneTest31(const stdstring& testName);
void RayPlaneTest32(const stdstring& testName);
void RayPlaneTest33(const stdstring& testName);
void RayPlaneTest34(const stdstring& testName);
void RayPlaneTest35(const stdstring& testName);
void RayPlaneTest36(const stdstring& testName);
void RayPlaneTest37(const stdstring& testName);
void RayPlaneTest38(const stdstring& testName);
void RayPlaneTest39(const stdstring& testName);
void RayPlaneTest40(const stdstring& testName);
void RayPlaneTest41(const stdstring& testName);
void RayPlaneTest42(const stdstring& testName);
void RayPlaneTest43(const stdstring& testName);
void RayPlaneTest44(const stdstring& testName);
void RayPlaneTest45(const stdstring& testName);
void RayPlaneTest46(const stdstring& testName);
void RayPlaneTest47(const stdstring& testName);
void RayPlaneTest48(const stdstring& testName);
void RayPlaneTest49(const stdstring& testName);
void RayPlaneTest50(const stdstring& testName);
void RayPlaneTest51(const stdstring& testName);
void RayPlaneTest52(const stdstring& testName);
void RayPlaneTest53(const stdstring& testName);
void RayPlaneTest54(const stdstring& testName);
void RayPlaneTest55(const stdstring& testName);
void RayPlaneTest56(const stdstring& testName);
void RayPlaneTest57(const stdstring& testName);
void RayPlaneTest58(const stdstring& testName);
void RayPlaneTest59(const stdstring& testName);
void RayPlaneTest60(const stdstring& testName);
void RayPlaneTest61(const stdstring& testName);
void RayPlaneTest62(const stdstring& testName);
void RayPlaneTest63(const stdstring& testName);
void RayPlaneTest64(const stdstring& testName);
void RayPlaneTest65(const stdstring& testName);
void RayPlaneTest66(const stdstring& testName);
void RayPlaneTest67(const stdstring& testName);
void RayPlaneTest68(const stdstring& testName);
void RayPlaneTest69(const stdstring& testName);
void RayPlaneTest70(const stdstring& testName);
void RayPlaneTest71(const stdstring& testName);
void RayPlaneTest72(const stdstring& testName);

void RayTriangleTest1(const stdstring& testName);
void RayTriangleTest2(const stdstring& testName);
void RayTriangleTest3(const stdstring& testName);
void RayTriangleTest4(const stdstring& testName);
void RayTriangleTest5(const stdstring& testName);
void RayTriangleTest6(const stdstring& testName);
void RayTriangleTest7(const stdstring& testName);
void RayTriangleTest8(const stdstring& testName);
void RayTriangleTest9(const stdstring& testName);
void RayTriangleTest10(const stdstring& testName);
void RayTriangleTest11(const stdstring& testName);
void RayTriangleTest12(const stdstring& testName);
void RayTriangleTest13(const stdstring& testName);
void RayTriangleTest14(const stdstring& testName);
void RayTriangleTest15(const stdstring& testName);
void RayTriangleTest16(const stdstring& testName);
void RayTriangleTest17(const stdstring& testName);
void RayTriangleTest18(const stdstring& testName);
void RayTriangleTest19(const stdstring& testName);
void RayTriangleTest20(const stdstring& testName);
void RayTriangleTest21(const stdstring& testName);
void RayTriangleTest22(const stdstring& testName);
void RayTriangleTest23(const stdstring& testName);
void RayTriangleTest24(const stdstring& testName);
void RayTriangleTest25(const stdstring& testName);
void RayTriangleTest26(const stdstring& testName);
void RayTriangleTest27(const stdstring& testName);
void RayTriangleTest28(const stdstring& testName);
void RayTriangleTest29(const stdstring& testName);
void RayTriangleTest30(const stdstring& testName);

void RaySphereTest1(const stdstring& testName);
void RaySphereTest2(const stdstring& testName);
void RaySphereTest3(const stdstring& testName);
void RaySphereTest4(const stdstring& testName);
void RaySphereTest5(const stdstring& testName);
void RaySphereTest6(const stdstring& testName);
void RaySphereTest7(const stdstring& testName);
void RaySphereTest8(const stdstring& testName);
void RaySphereTest9(const stdstring& testName);
void RaySphereTest10(const stdstring& testName);
void RaySphereTest11(const stdstring& testName);
void RaySphereTest12(const stdstring& testName);
void RaySphereTest13(const stdstring& testName);
void RaySphereTest14(const stdstring& testName);
void RaySphereTest15(const stdstring& testName);
void RaySphereTest16(const stdstring& testName);
void RaySphereTest17(const stdstring& testName);
void RaySphereTest18(const stdstring& testName);
void RaySphereTest19(const stdstring& testName);
void RaySphereTest20(const stdstring& testName);
void RaySphereTest21(const stdstring& testName);
void RaySphereTest22(const stdstring& testName);
void RaySphereTest23(const stdstring& testName);
void RaySphereTest24(const stdstring& testName);
void RaySphereTest25(const stdstring& testName);
void RaySphereTest26(const stdstring& testName);
void RaySphereTest27(const stdstring& testName);
void RaySphereTest28(const stdstring& testName);
void RaySphereTest29(const stdstring& testName);
void RaySphereTest30(const stdstring& testName);
void RaySphereTest31(const stdstring& testName);
void RaySphereTest32(const stdstring& testName);
void RaySphereTest33(const stdstring& testName);
void RaySphereTest34(const stdstring& testName);
void RaySphereTest35(const stdstring& testName);
void RaySphereTest36(const stdstring& testName);
void RaySphereTest37(const stdstring& testName);
void RaySphereTest38(const stdstring& testName);
void RaySphereTest39(const stdstring& testName);
void RaySphereTest40(const stdstring& testName);
void RaySphereTest41(const stdstring& testName);
void RaySphereTest42(const stdstring& testName);
void RaySphereTest43(const stdstring& testName);
void RaySphereTest44(const stdstring& testName);
void RaySphereTest45(const stdstring& testName);
void RaySphereTest46(const stdstring& testName);
void RaySphereTest47(const stdstring& testName);
void RaySphereTest48(const stdstring& testName);
void RaySphereTest49(const stdstring& testName);
void RaySphereTest50(const stdstring& testName);
void RaySphereTest51(const stdstring& testName);
void RaySphereTest52(const stdstring& testName);
void RaySphereTest53(const stdstring& testName);
void RaySphereTest54(const stdstring& testName);
void RaySphereTest55(const stdstring& testName);
void RaySphereTest56(const stdstring& testName);
void RaySphereTest57(const stdstring& testName);
void RaySphereTest58(const stdstring& testName);
void RaySphereTest59(const stdstring& testName);
void RaySphereTest60(const stdstring& testName);
void RaySphereTest61(const stdstring& testName);
void RaySphereTest62(const stdstring& testName);
void RaySphereTest63(const stdstring& testName);
void RaySphereTest64(const stdstring& testName);
void RaySphereTest65(const stdstring& testName);
void RaySphereTest66(const stdstring& testName);
void RaySphereTest67(const stdstring& testName);
void RaySphereTest68(const stdstring& testName);
void RaySphereTest69(const stdstring& testName);
void RaySphereTest70(const stdstring& testName);
void RaySphereTest71(const stdstring& testName);
void RaySphereTest72(const stdstring& testName);
void RaySphereTest73(const stdstring& testName);
void RaySphereTest74(const stdstring& testName);
void RaySphereTest75(const stdstring& testName);
void RaySphereTest76(const stdstring& testName);
void RaySphereTest77(const stdstring& testName);
void RaySphereTest78(const stdstring& testName);
void RaySphereTest79(const stdstring& testName);
void RaySphereTest80(const stdstring& testName);
void RaySphereTest81(const stdstring& testName);
void RaySphereTest82(const stdstring& testName);
void RaySphereTest83(const stdstring& testName);
void RaySphereTest84(const stdstring& testName);
void RaySphereTest85(const stdstring& testName);

void RayAabbTest1(const stdstring& testName);
void RayAabbTest2(const stdstring& testName);
void RayAabbTest3(const stdstring& testName);
void RayAabbTest4(const stdstring& testName);
void RayAabbTest5(const stdstring& testName);
void RayAabbTest6(const stdstring& testName);
void RayAabbTest7(const stdstring& testName);
void RayAabbTest8(const stdstring& testName);
void RayAabbTest9(const stdstring& testName);
void RayAabbTest10(const stdstring& testName);
void RayAabbTest11(const stdstring& testName);
void RayAabbTest12(const stdstring& testName);
void RayAabbTest13(const stdstring& testName);
void RayAabbTest14(const stdstring& testName);
void RayAabbTest15(const stdstring& testName);
void RayAabbTest16(const stdstring& testName);
void RayAabbTest17(const stdstring& testName);
void RayAabbTest18(const stdstring& testName);
void RayAabbTest19(const stdstring& testName);
void RayAabbTest20(const stdstring& testName);
void RayAabbTest21(const stdstring& testName);
void RayAabbTest22(const stdstring& testName);
void RayAabbTest23(const stdstring& testName);
void RayAabbTest24(const stdstring& testName);
void RayAabbTest25(const stdstring& testName);
void RayAabbTest26(const stdstring& testName);
void RayAabbTest27(const stdstring& testName);
void RayAabbTest28(const stdstring& testName);
void RayAabbTest29(const stdstring& testName);
void RayAabbTest30(const stdstring& testName);
void RayAabbTest31(const stdstring& testName);
void RayAabbTest32(const stdstring& testName);
void RayAabbTest33(const stdstring& testName);
void RayAabbTest34(const stdstring& testName);
void RayAabbTest35(const stdstring& testName);
void RayAabbTest36(const stdstring& testName);
void RayAabbTest37(const stdstring& testName);
void RayAabbTest38(const stdstring& testName);
void RayAabbTest39(const stdstring& testName);
void RayAabbTest40(const stdstring& testName);
void RayAabbTest41(const stdstring& testName);
void RayAabbTest42(const stdstring& testName);
void RayAabbTest43(const stdstring& testName);
void RayAabbTest44(const stdstring& testName);
void RayAabbTest45(const stdstring& testName);
void RayAabbTest46(const stdstring& testName);
void RayAabbTest47(const stdstring& testName);
void RayAabbTest48(const stdstring& testName);
void RayAabbTest49(const stdstring& testName);
void RayAabbTest50(const stdstring& testName);
void RayAabbTest51(const stdstring& testName);
void RayAabbTest52(const stdstring& testName);
void RayAabbTest53(const stdstring& testName);
void RayAabbTest54(const stdstring& testName);
void RayAabbTest55(const stdstring& testName);
void RayAabbTest56(const stdstring& testName);
void RayAabbTest57(const stdstring& testName);
void RayAabbTest58(const stdstring& testName);
void RayAabbTest59(const stdstring& testName);
void RayAabbTest60(const stdstring& testName);
void RayAabbTest61(const stdstring& testName);
void RayAabbTest62(const stdstring& testName);
void RayAabbTest63(const stdstring& testName);
void RayAabbTest64(const stdstring& testName);
void RayAabbTest65(const stdstring& testName);
void RayAabbTest66(const stdstring& testName);
void RayAabbTest67(const stdstring& testName);
void RayAabbTest68(const stdstring& testName);
void RayAabbTest69(const stdstring& testName);
void RayAabbTest70(const stdstring& testName);
void RayAabbTest71(const stdstring& testName);
void RayAabbTest72(const stdstring& testName);
void RayAabbTest73(const stdstring& testName);
void RayAabbTest74(const stdstring& testName);
void RayAabbTest75(const stdstring& testName);
void RayAabbTest76(const stdstring& testName);
void RayAabbTest77(const stdstring& testName);
void RayAabbTest78(const stdstring& testName);
void RayAabbTest79(const stdstring& testName);
void RayAabbTest80(const stdstring& testName);
void RayAabbTest81(const stdstring& testName);
void RayAabbTest82(const stdstring& testName);
void RayAabbTest83(const stdstring& testName);
void RayAabbTest84(const stdstring& testName);
void RayAabbTest85(const stdstring& testName);
void RayAabbTest86(const stdstring& testName);
void RayAabbTest87(const stdstring& testName);
void RayAabbTest88(const stdstring& testName);
void RayAabbTest89(const stdstring& testName);
void RayAabbTest90(const stdstring& testName);
void RayAabbTest91(const stdstring& testName);

void PlaneSphereTest1(const stdstring& testName);
void PlaneSphereTest2(const stdstring& testName);
void PlaneSphereTest3(const stdstring& testName);
void PlaneSphereTest4(const stdstring& testName);
void PlaneSphereTest5(const stdstring& testName);
void PlaneSphereTest6(const stdstring& testName);
void PlaneSphereTest7(const stdstring& testName);
void PlaneSphereTest8(const stdstring& testName);
void PlaneSphereTest9(const stdstring& testName);
void PlaneSphereTest10(const stdstring& testName);
void PlaneSphereTest11(const stdstring& testName);
void PlaneSphereTest12(const stdstring& testName);
void PlaneSphereTest13(const stdstring& testName);
void PlaneSphereTest14(const stdstring& testName);
void PlaneSphereTest15(const stdstring& testName);
void PlaneSphereTest16(const stdstring& testName);
void PlaneSphereTest17(const stdstring& testName);
void PlaneSphereTest18(const stdstring& testName);
void PlaneSphereTest19(const stdstring& testName);
void PlaneSphereTest20(const stdstring& testName);

void PlaneAabbTest1(const stdstring& testName);
void PlaneAabbTest2(const stdstring& testName);
void PlaneAabbTest3(const stdstring& testName);
void PlaneAabbTest4(const stdstring& testName);
void PlaneAabbTest5(const stdstring& testName);
void PlaneAabbTest6(const stdstring& testName);
void PlaneAabbTest7(const stdstring& testName);
void PlaneAabbTest8(const stdstring& testName);
void PlaneAabbTest9(const stdstring& testName);
void PlaneAabbTest10(const stdstring& testName);
void PlaneAabbTest11(const stdstring& testName);
void PlaneAabbTest12(const stdstring& testName);
void PlaneAabbTest13(const stdstring& testName);
void PlaneAabbTest14(const stdstring& testName);
void PlaneAabbTest15(const stdstring& testName);
void PlaneAabbTest16(const stdstring& testName);
void PlaneAabbTest17(const stdstring& testName);
void PlaneAabbTest18(const stdstring& testName);
void PlaneAabbTest19(const stdstring& testName);
void PlaneAabbTest20(const stdstring& testName);
void PlaneAabbTest21(const stdstring& testName);
void PlaneAabbTest22(const stdstring& testName);
void PlaneAabbTest23(const stdstring& testName);
void PlaneAabbTest24(const stdstring& testName);
void PlaneAabbTest25(const stdstring& testName);
void PlaneAabbTest26(const stdstring& testName);
void PlaneAabbTest27(const stdstring& testName);
void PlaneAabbTest28(const stdstring& testName);
void PlaneAabbTest29(const stdstring& testName);
void PlaneAabbTest30(const stdstring& testName);
void PlaneAabbTest31(const stdstring& testName);
void PlaneAabbTest32(const stdstring& testName);
void PlaneAabbTest33(const stdstring& testName);
void PlaneAabbTest34(const stdstring& testName);
void PlaneAabbTest35(const stdstring& testName);
void PlaneAabbTest36(const stdstring& testName);
void PlaneAabbTest37(const stdstring& testName);
void PlaneAabbTest38(const stdstring& testName);
void PlaneAabbTest39(const stdstring& testName);
void PlaneAabbTest40(const stdstring& testName);
void PlaneAabbTest41(const stdstring& testName);
void PlaneAabbTest42(const stdstring& testName);
void PlaneAabbTest43(const stdstring& testName);
void PlaneAabbTest44(const stdstring& testName);
void PlaneAabbTest45(const stdstring& testName);
void PlaneAabbTest46(const stdstring& testName);
void PlaneAabbTest47(const stdstring& testName);
void PlaneAabbTest48(const stdstring& testName);
void PlaneAabbTest49(const stdstring& testName);
void PlaneAabbTest50(const stdstring& testName);

#pragma endregion

