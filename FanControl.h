#pragma once
#include <windows.h>
#include <pdh.h>
#pragma comment(lib, "pdh.lib")

typedef BOOL(WINAPI* pRdmsr)(DWORD, PDWORD, PDWORD);

class FanControl {
public:
	FanControl();
	~FanControl();

	bool Initialize();
	void SetFanSpeed(int percent);
	void SetAutoMode();
	double GetPackageWattage();
	double GetCpuTemp();

private:
	HMODULE hLib;
	void WaitEC();

	pRdmsr Rdmsr;
	HQUERY hQuery = NULL;
	HCOUNTER hCounter = NULL;
	bool pdhInitialized = false;
};
