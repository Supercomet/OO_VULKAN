#pragma once

#include <Windows.h>
//----------------------------------------------------------------------------------
// Process Window Message Callbacks
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

struct Window
{
    Window(uint32_t width =400u, uint32_t height =400u);
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    ~Window();
	void Init();

    HWND GetRawHandle()const;


    uint32_t m_width;
    uint32_t m_height;
    HWND rawHandle;
    
};

