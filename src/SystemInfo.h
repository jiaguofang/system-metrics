#ifndef SystemInfo_h
#define SystemInfo_h

#ifdef _WIN32
#define _WIN32_WINNT (0x0501)
#include <windows.h>
#endif

class SystemInfo
{
public:
	SystemInfo();
	~SystemInfo() throw();

	double GetSystemCPUUsage();
	double GetSystemMemoryTotal();
	double GetSystemMemoryUsed();

private:
#ifdef _WIN32
	ULARGE_INTEGER mPrevSysIdleTime;	// the amount of time that the system has been idle.
	ULARGE_INTEGER mPrevSysKernelTime;	// the amount of time previously measured that the system
										// has spent executing in Kernel mode (including all threads
										// in all processes, on all processors). This time value also
										// includes the amount of time the system has been idle.
	ULARGE_INTEGER mPrevSysUserTime;	// the amount of time previously measured that the system
										// has spent executing in User mode (including all threads
										// in all processes, on all processors).

#elif defined(__linux__)
	unsigned long long mPrevSysUserTime;
	unsigned long long mPrevSysNiceTime;
	unsigned long long mPrevSysKernelTime;
	unsigned long long mPrevSysIdleTime;
#endif
};

#endif