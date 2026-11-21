#include "FanControl.h"
#include <iostream>
#include <cstdint>
#include <windows.h>
#include "nvml.h"

typedef BOOL(WINAPI* pInit)();
typedef void (WINAPI* pWrite)(WORD, BYTE);
typedef BYTE(WINAPI* pRead)(WORD);
typedef BOOL(WINAPI* pRdmsr)(DWORD, PDWORD, PDWORD);
typedef BOOL(WINAPI* pWritePciEx)(DWORD, DWORD, DWORD);
typedef BOOL(WINAPI* pReadPciEx)(DWORD, DWORD, PDWORD);

pInit   InitializeOls;
pWrite  WriteIoPortByte;
pRead   ReadIoPortByte;
pWritePciEx WritePciConfigDwordEx;
pReadPciEx  ReadPciConfigDwordEx;

FanControl::FanControl() { hLib = nullptr; }

FanControl::~FanControl() {
	if (hLib) FreeLibrary(hLib);
}

void FanControl::WaitEC() {
	if (!ReadIoPortByte) return;
	while (ReadIoPortByte(0x66) & 0x02)
		Sleep(1);
}

void FanControl::SetFanSpeed(int percent) {
	if (!WriteIoPortByte || !ReadIoPortByte) return;
	if (percent < 38) percent = 38;
	if (percent > 100) percent = 100;

	WaitEC(); WriteIoPortByte(0x66, 0x81);
	WaitEC(); WriteIoPortByte(0x62, 0x2D);
	WaitEC(); WriteIoPortByte(0x62, (BYTE)percent);

	WaitEC(); WriteIoPortByte(0x66, 0x81);
	WaitEC(); WriteIoPortByte(0x62, 0x35);
	WaitEC(); WriteIoPortByte(0x62, (BYTE)percent);
}

void FanControl::SetAutoMode() {
	if (!WriteIoPortByte || !ReadIoPortByte) return;

	WaitEC(); WriteIoPortByte(0x66, 0x81);
	WaitEC(); WriteIoPortByte(0x62, 0x2D);
	WaitEC(); WriteIoPortByte(0x62, 0x00);

	WaitEC(); WriteIoPortByte(0x66, 0x81);
	WaitEC(); WriteIoPortByte(0x62, 0x2E);
	WaitEC(); WriteIoPortByte(0x62, 0x00);
}

bool FanControl::Initialize() {
	hLib = LoadLibraryA("WinRing0x64.dll");
	if (!hLib) return false;

	Rdmsr           = (pRdmsr)GetProcAddress(hLib, "Rdmsr");
	InitializeOls   = (pInit) GetProcAddress(hLib, "InitializeOls");
	WriteIoPortByte = (pWrite)GetProcAddress(hLib, "WriteIoPortByte");
	ReadIoPortByte  = (pRead) GetProcAddress(hLib, "ReadIoPortByte");
	WritePciConfigDwordEx = (pWritePciEx)GetProcAddress(hLib, "WritePciConfigDwordEx");
	ReadPciConfigDwordEx  = (pReadPciEx)GetProcAddress(hLib, "ReadPciConfigDwordEx");

	if (!InitializeOls || !Rdmsr || !WriteIoPortByte || !ReadIoPortByte) return false;
	return InitializeOls();
}

double FanControl::GetCpuTemp() {
    // Try SMN register first (newer APUs, Renoir/Lucienne)
    if (WritePciConfigDwordEx && ReadPciConfigDwordEx) {
        DWORD smnAddr = 0x00059800; // THM_TCON_CUR_TMP
        DWORD smnData = 0;
        DWORD pciAddress = 0; // Bus 0, Dev 0, Func 0
        
        // Write SMN address to index register (0xB8)
        if (WritePciConfigDwordEx(pciAddress, 0xB8, smnAddr)) {
            // Read SMN data from data register (0xBC)
            if (ReadPciConfigDwordEx(pciAddress, 0xBC, &smnData)) {
                bool rangeSel = (smnData >> 19) & 1;
                DWORD rawTemp = (smnData >> 21) & 0x7FF;
                double temp = rangeSel ? (rawTemp / 8.0) - 49.0 : (rawTemp / 8.0);
                if (temp > 0.0 && temp < 150.0) return temp;
            }
        }
    }

    // Fallback to MSR (Zen/Zen+)
    if (Rdmsr) {
        DWORD eax = 0, edx = 0;
        if (Rdmsr(0xC0010299, &eax, &edx)) {
            double temp = ((eax >> 21) & 0x7FF) * 0.125;
            if (temp > 0.0 && temp < 150.0) return temp;
        }
    }
    
    return 0.0;
}

double FanControl::GetPackageWattage() {
	if (!Rdmsr) return 0.0;

	static uint64_t lastEnergy = 0;
	static LARGE_INTEGER lastTime = {};
	static LARGE_INTEGER frequency = {};

	DWORD eax, edx;
	if (!Rdmsr(0xC001029B, &eax, &edx)) return 0.0;

	if (frequency.QuadPart == 0) QueryPerformanceFrequency(&frequency);

	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);

	if (lastEnergy == 0) {
		lastEnergy = (uint64_t)eax;
		lastTime = currentTime;
		return 0.0;
	}

	double deltaTime = (double)(currentTime.QuadPart - lastTime.QuadPart) / (double)frequency.QuadPart;
	double wattage = (((uint64_t)eax - lastEnergy) / 65536.0) / deltaTime;

	lastEnergy = (uint64_t)eax;
	lastTime = currentTime;

	return (wattage > 0.0 && wattage < 200.0) ? wattage : 0.0;
}
