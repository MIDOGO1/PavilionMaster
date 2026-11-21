// Sits in front of the real RyzenADJ. Pavilion Master calls "ryzenadj.exe" once per mode with a
// distinct --tctl-temp (92 = Performance, 85 = Balanced, 75 = Quiet). This forwards the call to the
// real tool, then switches the Windows power plan to match.
#include <windows.h>
#include <string>

static const wchar_t *SAVER    = L"a1841308-3541-4fab-bc81-f71556f20b4a";
static const wchar_t *BALANCED = L"381b4222-f694-41f0-9685-ff5bb260df2e";
static const wchar_t *HIGHPERF = L"8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c";
static const wchar_t *ULTIMATE = L"e9a42b02-d5df-448d-aa00-03f14749eb61";
static const wchar_t *PM_GUID  = L"5a7c1e2b-9d34-4f6a-8b1c-0e7d2f4a6b93";   // our own plan

static DWORD runHidden(const std::wstring &cmd, const std::wstring &dir, DWORD waitMs = 8000)
{
    STARTUPINFOW si{}; si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::wstring c = cmd;
    if (!CreateProcessW(nullptr, &c[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr,
                        dir.empty() ? nullptr : dir.c_str(), &si, &pi))
        return 0xFFFFFFFF;
    WaitForSingleObject(pi.hProcess, waitMs);
    DWORD code = 1; GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
    return code;
}

static std::wstring powercfgExe()
{
    wchar_t sys[MAX_PATH]; GetSystemDirectoryW(sys, MAX_PATH);
    return std::wstring(L"\"") + sys + L"\\powercfg.exe\" ";
}
static DWORD pc(const std::wstring &args) { return runHidden(powercfgExe() + args, L""); }

static void activate(const wchar_t *guid)
{
    std::wstring g(guid);
    if (pc(L"/setactive " + g) == 0) return;
    pc(L"/duplicatescheme " + g);               // restore a built-in plan that was removed
    pc(L"/setactive " + g);
}

static void activateUltimate()
{
    if (pc(std::wstring(L"/setactive ") + PM_GUID) == 0) return;       // already created
    std::wstring name = L"Pavilion Master Ultimate Performance";
    if (pc(std::wstring(L"/duplicatescheme ") + ULTIMATE + L" " + PM_GUID) != 0) {
        if (pc(std::wstring(L"/duplicatescheme ") + HIGHPERF + L" " + PM_GUID) != 0) {
            activate(HIGHPERF);                                       // last resort
            return;
        }
        name = L"Pavilion Master Performance";
    }
    pc(std::wstring(L"/changename ") + PM_GUID + L" \"" + name + L"\" \"Created by Pavilion Master\"");
    pc(std::wstring(L"/setactive ") + PM_GUID);
}

static std::wstring afterFirstToken(const std::wstring &cl)
{
    size_t i = 0;
    if (!cl.empty() && cl[0] == L'"') { i = cl.find(L'"', 1); i = (i == std::wstring::npos) ? cl.size() : i + 1; }
    else { i = cl.find(L' '); if (i == std::wstring::npos) i = cl.size(); }
    return cl.substr(i);
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring dir(path);
    dir.erase(dir.find_last_of(L'\\'));

    const std::wstring args = afterFirstToken(GetCommandLineW());
    DWORD rc = runHidden(L"\"" + dir + L"\\ryzenadj-real.exe\"" + args, dir, 30000);

    size_t p = args.find(L"--tctl-temp=");
    if (p != std::wstring::npos) {
        int tctl = _wtoi(args.c_str() + p + 12);
        if      (tctl == 92) activateUltimate();      // Performance
        else if (tctl == 85) activate(BALANCED);      // Balanced
        else if (tctl == 75) activate(SAVER);         // Quiet
    }
    return (int)rc;
}
