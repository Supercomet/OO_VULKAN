#pragma once

#include <vector>
#include <memory>

class TestApplication
{
public:

    void Init();
    void Run();

private:

    void InitDefaultTextures();
    void InitDefaultMeshes();

    bool m_TestDebugDrawBox{ false };
    bool m_TestDebugDrawDisc{ false };
    bool m_TestDebugDrawDecal{ false };
};
