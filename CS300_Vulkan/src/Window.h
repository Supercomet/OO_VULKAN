#pragma once

#include <Windows.h>
//----------------------------------------------------------------------------------
// Process Window Message Callbacks
LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

class Window
{
public :
    Window(uint32_t width =400u, uint32_t height =400u);
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
	void Init();

    HWND GetRawHandle()const;

private:

    uint32_t m_width;
    uint32_t m_height;
    HWND rawHandle;
    
};

