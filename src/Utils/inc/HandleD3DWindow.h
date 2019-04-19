#ifndef HANDLE_D3D_WINDOW
#define HANDLE_D3D_WINDOW

#include <windows.h>
#include <dxgi1_2.h>
#include <d3d11.h>


struct WindowData {
    unsigned long pid;
    HWND wHandle;
};

HWND GetMainWindow (unsigned long pid);
BOOL CALLBACK EnumWindowsCallback (HWND handle, LPARAM lparam);
BOOL IsMainWindow (HWND handle);

#endif