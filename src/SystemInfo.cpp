#include "SystemInfo.h"

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
	// get amount of time the system has run in kernel and user mode
	FILETIME lSysIdleTime, lSysKernelTime, lSysUserTime;
	BOOL lSuccess = GetSystemTimes(&lSysIdleTime, &lSysKernelTime, &lSysUserTime);
	if (lSuccess)
	{
		memcpy(&mPrevSysIdleTime, &lSysIdleTime, sizeof(FILETIME));
		memcpy(&mPrevSysKernelTime, &lSysKernelTime, sizeof(FILETIME));
		memcpy(&mPrevSysUserTime, &lSysUserTime, sizeof(FILETIME));
	}

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
	// get amount of time the system has run in kernel and user mode
	FILETIME lSysIdleTime, lSysKernelTime, lSysUserTime;
	BOOL lSuccess = GetSystemTimes(&lSysIdleTime, &lSysKernelTime, &lSysUserTime);
	ULARGE_INTEGER lCurrSysIdleTime, lCurrSysKernelTime, lCurrSysUserTime;
	if (lSuccess)
	{
		memcpy(&lCurrSysIdleTime, &lSysIdleTime, sizeof(FILETIME));
		memcpy(&lCurrSysKernelTime, &lSysKernelTime, sizeof(FILETIME));
		memcpy(&lCurrSysUserTime, &lSysUserTime, sizeof(FILETIME));
	}

	// calculate system cpu usage
	ULONGLONG lTotalBusy = (lCurrSysKernelTime.QuadPart + lCurrSysUserTime.QuadPart - lCurrSysIdleTime.QuadPart) -
		(mPrevSysKernelTime.QuadPart + mPrevSysUserTime.QuadPart - mPrevSysIdleTime.QuadPart);
	ULONGLONG lTotalSystem = (lCurrSysKernelTime.QuadPart + lCurrSysUserTime.QuadPart) -
		(mPrevSysKernelTime.QuadPart + mPrevSysUserTime.QuadPart);
	if (lTotalSystem > 0)
		lCPUUsage = lTotalBusy * 100.0 / lTotalSystem;
	
	// store current time info
	mPrevSysIdleTime = lCurrSysIdleTime;
	mPrevSysKernelTime = lCurrSysKernelTime;
	mPrevSysUserTime = lCurrSysUserTime;

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
