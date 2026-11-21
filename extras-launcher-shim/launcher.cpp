// Pavilion Master launcher: enforces a single running instance, then starts the app.
#include <windows.h>
#include <string>

static const wchar_t *kTitle = L"HP Pavilion 15 Gaming Control Center";

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    HANDLE mutex = CreateMutexW(nullptr, FALSE, L"Local\\PavilionMaster_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // Already running: bring the existing window to the front and quit
        if (HWND w = FindWindowW(nullptr, kTitle)) {
            ShowWindow(w, IsIconic(w) ? SW_RESTORE : SW_SHOW);
            SetForegroundWindow(w);
        }
        return 0;
    }

    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring dir(path);
    dir.erase(dir.find_last_of(L'\\'));
    std::wstring core = dir + L"\\PavilionMasterCore.exe";

    STARTUPINFOW si{}; si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::wstring cmd = L"\"" + core + L"\"";
    if (!CreateProcessW(core.c_str(), &cmd[0], nullptr, nullptr, FALSE, 0, nullptr, dir.c_str(), &si, &pi)) {
        MessageBoxW(nullptr, L"Could not start PavilionMasterCore.exe. Reinstall Pavilion Master.",
                    L"Pavilion Master", MB_ICONERROR);
        return 1;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);   // keep the mutex until the app closes
    CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
    CloseHandle(mutex);
    return 0;
}
