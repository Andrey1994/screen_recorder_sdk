#include "HandleD3DWindow.h"


HWND GetMainWindow (unsigned long pid)
{
    WindowData data;
    data.pid = pid;
    data.wHandle = 0;
    EnumWindows (EnumWindowsCallback, (LPARAM)&data);
    return data.wHandle;
}

BOOL CALLBACK EnumWindowsCallback (HWND handle, LPARAM lparam)
{
    WindowData& data = *(WindowData*)lparam;
    unsigned long pid = 0;
    GetWindowThreadProcessId (handle, &pid);
    if (data.pid != pid || !IsMainWindow (handle))
        return TRUE;
    data.wHandle = handle;
    return FALSE;
}

BOOL IsMainWindow (HWND handle)
{
    return GetWindow (handle, GW_OWNER) == (HWND)0 && IsWindowVisible (handle);
}
