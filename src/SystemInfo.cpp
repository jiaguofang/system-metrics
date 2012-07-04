#include "SystemInfo.h"

#ifdef _WIN32
#pragma comment(lib, "pdh.lib")
#endif

#ifdef __linux__
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#endif

SystemInfo::SystemInfo()
{
#ifdef _WIN32
	PdhOpenQuery(NULL, NULL, &mQuery);
	PdhAddCounter(mQuery, L"\\Processor(_Total)\\% Processor Time", NULL, &mCounter);
	PdhCollectQueryData(mQuery);

#elif defined(__linux__)
	FILE* lpFile = fopen("/proc/stat", "r");
	if (lpFile)
	{
		fscanf(lpFile, "cpu %llu %llu %llu %llu", &mPrevUserTime, &mPrevNiceTime, &mPrevKernelTime, &mPrevIdleTime);
		fclose(lpFile);
	}
#endif
}

SystemInfo::~SystemInfo() throw()
{
}

double SystemInfo::GetSystemCPUUsage()
{
	double lCPUUsage = -1;

#ifdef _WIN32
	PDH_STATUS lStatus = PdhCollectQueryData(mQuery);
	if (lStatus == ERROR_SUCCESS)
	{
		PDH_FMT_COUNTERVALUE lCounterVal;
		lStatus = PdhGetFormattedCounterValue(mCounter, PDH_FMT_DOUBLE, NULL, &lCounterVal);
		if (lStatus == ERROR_SUCCESS)
			lCPUUsage = lCounterVal.doubleValue;
	}

#elif defined(__linux__)
	unsigned long long lUserTime, lNiceTime, lKernelTime, lIdleTime;

	FILE* lpFile = fopen("/proc/stat", "r");
	if (lpFile)
	{
		fscanf(lpFile, "cpu %llu %llu %llu %llu", &lUserTime, &lNiceTime, &lKernelTime, &lIdleTime);
		fclose(lpFile);

		unsigned long long lNotIdle = (lUserTime - mPrevUserTime) + (lNiceTime - mPrevNiceTime) + (lKernelTime - mPrevKernelTime);
		unsigned long long lTotalSystem = (lUserTime - mPrevUserTime) + (lNiceTime - mPrevNiceTime) + (lKernelTime - mPrevKernelTime) + (lIdleTime - mPrevIdleTime);
		// the measure time is too short
		if (lTotalSystem > 0)
			lCPUUsage = (lNotIdle * 100.0) / lTotalSystem;

		mPrevUserTime = lUserTime;
		mPrevNiceTime = lNiceTime;
		mPrevKernelTime = lKernelTime;
		mPrevIdleTime = lIdleTime;
	}

#endif
	return lCPUUsage;
}

double SystemInfo::GetSystemMemoryTotal()
{
	double lMemTotal = -1;

#ifdef _WIN32
	MEMORYSTATUSEX lMemInfo;
	lMemInfo.dwLength = sizeof(MEMORYSTATUSEX);
	BOOL lSuccess = GlobalMemoryStatusEx(&lMemInfo);

	// size in MB
	if (lSuccess)
		lMemTotal = lMemInfo.ullTotalPhys / (1024.0 * 1024.0);

#elif defined(__linux__)
	struct sysinfo lSysinfo;
	int lReturn = sysinfo(&lSysinfo);

	if (lReturn == 0)
		lMemTotal = (lSysinfo.totalram * lSysinfo.mem_unit) / (1024.0 * 1024.0);

#endif
	return lMemTotal;
}

double SystemInfo::GetSystemMemoryUsed()
{
	double lMemUsed = -1;

#ifdef _WIN32
	MEMORYSTATUSEX lMemInfo;
	lMemInfo.dwLength = sizeof(MEMORYSTATUSEX);
	BOOL lSuccess = GlobalMemoryStatusEx(&lMemInfo);

	// size in MB
	if (lSuccess)
		lMemUsed = (lMemInfo.ullTotalPhys - lMemInfo.ullAvailPhys) / (1024.0 * 1024.0);

#elif defined(__linux__)
	struct sysinfo lSysinfo;
	int lReturn = sysinfo(&lSysinfo);

	if (lReturn == 0)
		lMemUsed = (lSysinfo.totalram - lSysinfo.freeram) * lSysinfo.mem_unit / (1024.0 * 1024.0);

#endif
	return lMemUsed;
}
