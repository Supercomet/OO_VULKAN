#pragma once

#include "MathCommon.h"

#include <vector>
#include <memory>
#include <string>
#include "Font.h"

struct ModelFileResource;

class TestApplication
{
public:

    void Init();
    void Run();

    int32_t CreateTextHelper(glm::mat4 xform, std::string str, oGFX::Font* testFont);

private:

    glm::ivec2 m_WindowSize{};

    uint32_t m_ApplicationFrame{ 0 };
    float m_ApplicationTimer{ 0.0f };
    float m_ApplicationDT{ 0.0f };
    bool m_ShowImGuiDemoWindow{ false };

    void InitDefaultTextures();
    void InitDefaultMeshes();

    void LoadMeshTextures(ModelFileResource* model);
    void ProcessModelScene(ModelFileResource* model);

    void RunTest_DebugDraw();
    bool m_TestDebugDrawLine{ false };
    bool m_TestDebugDrawBox{ false };
    bool m_TestDebugDrawDisc{ false };
    bool m_TestDebugDrawDecal{ false };
    bool m_TestDebugDrawGrid{ true };
    bool m_TestDebugFrustum{ false };

    bool m_debugDrawLightPosition{ false };

    void Tool_HandleGizmoManipulation();
    void Tool_HandleUI();

    void ToolUI_Camera();
    void ToolUI_Settings();

    void TestFrustumCull(oGFX::Frustum f, oGFX::AABB box);
};
