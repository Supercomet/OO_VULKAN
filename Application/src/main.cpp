#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#endif

#include "gpuCommon.h"
#include "Vulkanrenderer.h"

#include "window.h"
#include "input.h"

#include "Tests_Assignment1.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/backends/imgui_impl_win32.h>
#include "ImGuizmo.h"

#include "IcoSphereCreator.h"
#include "Tree.h"

#include "Profiling.h"

#include "BoudingVolume.h"
#include "DefaultMeshCreator.h"

#include "AppUtils.h"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <cctype>
#include <thread>
#include <functional>
#include <random>
#include <numeric>
#include <algorithm>

#if 1
#include "TestApplication.h"
int main(int argc, char argv[])
{
    (void)argc;
    (void)argv;

    _CrtDumpMemoryLeaks();
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _CrtSetBreakAlloc(156);

    // !! IMPORTANT !!
    // !! THIS IS A HACK !!
    // - Hijacking the directory So that all the models/shaders/textures folder can be accessed
    // - This is a quick workaround so that this .exe from this project can be run from Visual Studio.
    // - As such, be careful when running from the .exe application directly.
    SetCurrentDirectory(L"../OO_Vulkan/");

    TestApplication app;
    app.Run();

    return 0;
}
#endif
