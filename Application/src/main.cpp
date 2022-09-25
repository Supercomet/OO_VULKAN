#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "AppUtils.h"
#include "TestApplication.h"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <cctype>
#include <thread>
#include <functional>
#include <random>
#include <numeric>
#include <algorithm>

int main(int argc, char argv[])
{
    (void)argc;
    (void)argv;

    //_CrtDumpMemoryLeaks();
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    //_CrtSetBreakAlloc(156);

    // !! IMPORTANT !!
    // !! THIS IS A HACK !!
    // - Hijacking the directory So that all the g_globalModels/shaders/textures folder can be accessed
    // - This is a quick workaround so that this .exe from this project can be run from Visual Studio.
    // - As such, be careful when running from the .exe application directly.
    SetCurrentDirectory(L"../OO_Vulkan/");

    auto app = std::make_unique<TestApplication>();
    app->Run();

    if constexpr (false) // Simulate a memory leak
    {
        int* p = new int;
    }

    return 0;
}
