#include "ProcessInfo.h"

#ifdef _WIN32
#include "psapi.h"
#pragma comment(lib, "psapi.lib")
#include "tlhelp32.h"
#endif

#ifdef __linux__
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#define LINEBUFFLEN 2048
#endif

ProcessInfo::ProcessInfo(unsigned int iProcessId)
{
	mProcessId = iProcessId;

#ifdef _WIN32
	HANDLE lProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, mProcessId);
	if (lProcessHandle != NULL)
	{
		// get amount of time the system has run in kernel and user mode
		FILETIME lSysIdleTime, lSysKernelTime, lSysUserTime;
		BOOL lSuccess = GetSystemTimes(&lSysIdleTime, &lSysKernelTime, &lSysUserTime);
		if (lSuccess)
		{
			memcpy(&mPrevSysKernelTime, &lSysKernelTime, sizeof(FILETIME));
			memcpy(&mPrevSysUserTime, &lSysUserTime, sizeof(FILETIME));
		}

		// get amount of time the process has ran in kernel and user mode
		FILETIME lProcCreationTime, lProcExitTime, lProcKernelTime, lProcUserTime;
		lSuccess = GetProcessTimes(lProcessHandle, &lProcCreationTime, &lProcExitTime, &lProcKernelTime, &lProcUserTime);
		if (lSuccess)
		{
			memcpy(&mCreationTime, &lProcCreationTime, sizeof(FILETIME));
			memcpy(&mPrevProcKernelTime, &lProcKernelTime, sizeof(FILETIME));
			memcpy(&mPrevProcUserTime, &lProcUserTime, sizeof(FILETIME));
		}

		CloseHandle(lProcessHandle);
	}

#elif defined(__linux__)
	mJiffiesPerSecond = sysconf(_SC_CLK_TCK);
	mPrevSystemTime = 0;
	mPrevProcUserTime = 0;
	mPrevProcKernelTime = 0;

	// calculate total system time from file /proc/stat,
	// the content is like: cpu 7967 550 4155 489328
	FILE* lpFile = fopen("/proc/stat", "r");
	if (lpFile)
	{
		// skip unnecessary content
		fscanf(lpFile, "cpu");
		unsigned long long lTime;
		int lValuesToRead = 4;
		for (int i = 0; i < lValuesToRead; i++)
		{
			fscanf(lpFile, "%llu", &lTime);
			mPrevSystemTime += lTime;
		}
		fclose(lpFile);
	}

	// get user mode time, kernel mode time, start time
	// for current process from file /proc/[pid]/stat
	char lFileName[256];
	sprintf(lFileName, "/proc/%d/stat", mProcessId);
	lpFile = fopen(lFileName, "r");
	if (lpFile)
	{
		// skip unnecessary content
		int lValuesToSkip = 13;
		char lTemp[LINEBUFFLEN];
		for (int i = 0; i < lValuesToSkip; i++)
			fscanf(lpFile, "%s", lTemp);
		fscanf(lpFile, "%llu %llu", &mPrevProcUserTime, &mPrevProcKernelTime);

		// skip unnecessary content
		lValuesToSkip = 6;
		for (int i = 0; i < lValuesToSkip; i++)
			fscanf(lpFile, "%s", lTemp);
		unsigned long long lStartTimeSinceBoot;
		fscanf(lpFile, "%llu", &lStartTimeSinceBoot);
		mStartTimeSinceBoot = lStartTimeSinceBoot / mJiffiesPerSecond;

		fclose(lpFile);
	}
#endif
}

ProcessInfo::~ProcessInfo() throw()
{
}

unsigned int ProcessInfo::GetProcessId()
{
	return mProcessId;
}

unsigned long long ProcessInfo::GetProcessUptime()
{
	unsigned long long lUptimeInSec = -1;

#ifdef _WIN32
	FILETIME lCurrTime;
	GetSystemTimeAsFileTime(&lCurrTime);
	
	ULARGE_INTEGER ulCurrTime;
	memcpy(&ulCurrTime, &lCurrTime, sizeof(FILETIME));

	// The FILETIME structure represents the number of 100-nanosecond intervals,
	// so we need to divide by 10 million to get actual seconds
	lUptimeInSec = (ulCurrTime.QuadPart - mCreationTime.QuadPart) / 10000000;

	return lUptimeInSec;

#elif defined(__linux__)
	struct sysinfo lSysinfo;
	int lReturn = sysinfo(&lSysinfo);

	if (lReturn == 0)
		lUptimeInSec = lSysinfo.uptime - mStartTimeSinceBoot;

#endif
	return lUptimeInSec;
}

double ProcessInfo::GetProcessCPUUsage()
{
	double lCPUUsage = -1;

#ifdef _WIN32
	HANDLE lProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, mProcessId);
	if (lProcessHandle != NULL)
	{
		// get amount of time the system has run in kernel and user mode
		FILETIME lSysIdleTime, lSysKernelTime, lSysUserTime;
		BOOL lSuccess = GetSystemTimes(&lSysIdleTime, &lSysKernelTime, &lSysUserTime);
		ULARGE_INTEGER lCurrSysKernelTime, lCurrSysUserTime;
		if (lSuccess)
		{
			memcpy(&lCurrSysKernelTime, &lSysKernelTime, sizeof(FILETIME));
			memcpy(&lCurrSysUserTime, &lSysUserTime, sizeof(FILETIME));
		}

		// get amount of time the process has ran in kernel and user mode
		FILETIME lProcCreationTime, lProcExitTime, lProcKernelTime, lProcUserTime;
		lSuccess = GetProcessTimes(lProcessHandle, &lProcCreationTime, &lProcExitTime, &lProcKernelTime, &lProcUserTime);
		if (lSuccess)
		{
			ULARGE_INTEGER lCurrProcKernelTime, lCurrProcUserTime;
			memcpy(&lCurrProcKernelTime, &lProcKernelTime, sizeof(FILETIME));
			memcpy(&lCurrProcUserTime, &lProcUserTime, sizeof(FILETIME));

			// calculate process cpu usage
			ULONGLONG lTotalProcess = (lCurrProcKernelTime.QuadPart - mPrevProcKernelTime.QuadPart)
				+ (lCurrProcUserTime.QuadPart - mPrevProcUserTime.QuadPart);
			ULONGLONG lTotalSystem = (lCurrSysKernelTime.QuadPart - mPrevSysKernelTime.QuadPart)
				+ (lCurrSysUserTime.QuadPart - mPrevSysUserTime.QuadPart);
			if (lTotalSystem > 0)
				lCPUUsage = lTotalProcess * 100.0 / lTotalSystem;
			
			// store current time info
			mPrevSysKernelTime = lCurrSysKernelTime;
			mPrevSysUserTime = lCurrSysUserTime;
			mPrevProcKernelTime = lCurrProcKernelTime;
			mPrevProcUserTime = lCurrProcUserTime;
		}

		CloseHandle(lProcessHandle);
	}

#elif defined(__linux__)
	unsigned long long lCurrSystemTime = 0;
	unsigned long long lCurrUserTime = 0;
	unsigned long long lCurrKernelTime = 0;

	// calculate total system time from file /proc/stat,
	// the content is like: cpu 7967 550 4155 489328
	FILE* lpFile = fopen("/proc/stat", "r");
	if (lpFile)
	{
		// skip unnecessary content
		fscanf(lpFile, "cpu");
		unsigned long long lTime;
		int lValuesToRead = 4;
		for (int i = 0; i < lValuesToRead; i++)
		{
			fscanf(lpFile, "%llu", &lTime);
			lCurrSystemTime += lTime;
		}
		fclose(lpFile);
	}

	// get user mode and kernel mode time for current
	// process from file /proc/[pid]/stat
	char lFileName[256];
	sprintf(lFileName, "/proc/%d/stat", mProcessId);
	lpFile = fopen(lFileName, "r");
	if (lpFile)
	{
		// skip unnecessary content
		char lTemp[LINEBUFFLEN];
		int lValuesToSkip = 13;
		for (int i = 0; i < lValuesToSkip; i++)
			fscanf(lpFile, "%s", lTemp);

		fscanf(lpFile, "%llu %llu", &lCurrUserTime, &lCurrKernelTime);
		fclose(lpFile);
	}

	unsigned long long lTotalProcess = (lCurrUserTime - mPrevProcUserTime) + (lCurrKernelTime - mPrevProcKernelTime);
	unsigned long long lTotalSystem = lCurrSystemTime - mPrevSystemTime;
	if (lTotalSystem > 0)
		lCPUUsage = (lTotalProcess * 100.0) / lTotalSystem;

	mPrevSystemTime = lCurrSystemTime;
	mPrevProcUserTime = lCurrUserTime;
	mPrevProcKernelTime = lCurrKernelTime;

#endif
	return lCPUUsage;
}

double ProcessInfo::GetProcessMemoryUsed()
{
	double lMemUsed = -1;

#ifdef _WIN32
	HANDLE lProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, mProcessId);
	if (lProcessHandle != NULL)
	{
		PROCESS_MEMORY_COUNTERS lPMC;
		BOOL lSuccess = GetProcessMemoryInfo(lProcessHandle, &lPMC, sizeof(lPMC));
		if (lSuccess)
		{
			// size in MB
			lMemUsed = lPMC.WorkingSetSize / (1024.0 * 1024.0);
		}

		CloseHandle(lProcessHandle);
	}

#elif defined(__linux__)
	char lFileName[256];
	sprintf(lFileName, "/proc/%d/status", mProcessId);
	FILE* lpFile = fopen(lFileName, "r");
	char lLineBuf[LINEBUFFLEN];
	if(lpFile)
	{
		while(fgets(lLineBuf, LINEBUFFLEN, lpFile))
		{
			if (0 == strncmp(lLineBuf, "VmRSS:", 6))
			{
				char* cursor = lLineBuf + 6;
				/* Get rid of preceding blanks */
				while (!isdigit(*cursor))
				{
					cursor++;
				}
				/* Get rid of following blanks */
				char* lNumString = cursor;
				while (isdigit(*cursor))
				{
					cursor++;
				}
				*cursor = '\0';
				lMemUsed = atoll(lNumString) / 1024.0;
				break;
			}
		}
		fclose(lpFile);
	}

#endif
	return lMemUsed;
}

unsigned long ProcessInfo::GetProcessThreadCount()
{
	unsigned long lThreadCnt = -1;

#ifdef _WIN32
	// get a process list snapshot
	HANDLE lSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);
	if (lSnapshot != NULL)
	{
		// initialize the process entry structure
		PROCESSENTRY32 lEntry;
		lEntry.dwSize = sizeof(PROCESSENTRY32);

		// get the first process info
		BOOL lSuccess = Process32First(lSnapshot, &lEntry);
		while (lSuccess)
		{
			if (lEntry.th32ProcessID == mProcessId)
			{
				lThreadCnt = lEntry.cntThreads;
				break;
			}
			lSuccess = Process32Next(lSnapshot, &lEntry);
		}

		CloseHandle(lSnapshot);
	}

#elif defined(__linux__)
	// the 20th value in file /proc/[pid]/stat:
	// num_threads %ld, Number of threads in this process
	char lFileName[256];
	sprintf(lFileName, "/proc/%d/stat", mProcessId);
	FILE* lpFile = fopen(lFileName, "r");
	if (lpFile)
	{
		// skip unnecessary content
		char lTemp[LINEBUFFLEN];
		int lValuesToSkip = 19;
		for (int i = 0; i < lValuesToSkip; i++)
			fscanf(lpFile, "%s", lTemp);
		fscanf(lpFile, "%lu", &lThreadCnt);
		fclose(lpFile);
	}

#endif
	return lThreadCnt;
}
