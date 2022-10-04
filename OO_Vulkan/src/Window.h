/************************************************************************************//*!
\file           Window.h
\project        Ouroboros
\author         Jamie Kong, j.kong, 390004720 | code contribution (100%)
\par            email: j.kong\@digipen.edu
\date           Oct 02, 2022
\brief          A window wrapper object

Copyright (C) 2022 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*//*************************************************************************************/
#pragma once

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
//----------------------------------------------------------------------------------
// Process Window Message Callbacks
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

struct Window
{

    enum WindowType
    {
        WINDOWS32,
        SDL2,


    };


    Window(uint32_t width =1024u, uint32_t height =720u, WindowType = WindowType::WINDOWS32);
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    ~Window();
	void Init();

    HWND GetRawHandle()const;

    static bool PollEvents();

    uint32_t m_width;
    uint32_t m_height;
    WindowType m_type;
    void* rawHandle;

    bool windowShouldClose;

    static uint64_t SurfaceFormat;
    
};

