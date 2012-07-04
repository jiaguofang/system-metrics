#ifndef SystemInfo_h
#define SystemInfo_h

#ifdef _WIN32
#include <pdh.h>
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
	PDH_HQUERY mQuery;
	PDH_HCOUNTER mCounter;

#elif defined(__linux__)
	unsigned long long mPrevUserTime;
	unsigned long long mPrevNiceTime;
	unsigned long long mPrevKernelTime;
	unsigned long long mPrevIdleTime;
#endif
};

#endif