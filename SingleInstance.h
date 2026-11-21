#pragma once
// Only one Pavilion Master at a time.
// Call acquireSingleInstance() at the very top of main(), before QGuiApplication.
#include <windows.h>

inline bool acquireSingleInstance(const wchar_t *windowTitle = L"Pavilion Master")
{
    static HANDLE mutex = nullptr;            // kept open for the life of the process
    mutex = CreateMutexW(nullptr, FALSE, L"Local\\PavilionMaster_App");
    const bool alreadyRunning = (GetLastError() == ERROR_ALREADY_EXISTS);

    if (alreadyRunning) {
        // Bring the existing window forward instead of silently doing nothing
        if (HWND w = FindWindowW(nullptr, windowTitle)) {
            ShowWindow(w, IsIconic(w) ? SW_RESTORE : SW_SHOW);
            SetForegroundWindow(w);
        }
        return false;
    }
    return true;
}
