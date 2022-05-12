#pragma once

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#endif
//----------------------------------------------------------------------------------
// Process Window Message Callbacks
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

struct Window
{
    Window(uint32_t width =800u, uint32_t height =800u);
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    ~Window();
	void Init();

    HWND GetRawHandle()const;

    static bool PollEvents();

    uint32_t m_width;
    uint32_t m_height;
    HWND rawHandle;

    bool windowShouldClose;
    
};

