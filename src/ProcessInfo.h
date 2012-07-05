#ifndef ProcessInfo_h
#define ProcessInfo_h

#ifdef _WIN32
#define _WIN32_WINNT (0x0501)
#include <windows.h>
#endif

class ProcessInfo
{
public:
	ProcessInfo(unsigned int iProcessId);
	~ProcessInfo() throw();

	unsigned int GetProcessId();
	unsigned long long GetProcessUptime();
	double GetProcessCPUUsage();
	double GetProcessMemoryUsed();
	unsigned long GetProcessThreadCount();

private:
	unsigned int mProcessId;
#ifdef _WIN32
	ULARGE_INTEGER mCreationTime;		// process creation time
	ULARGE_INTEGER mPrevSysKernelTime;	// the amount of time previously measured that the system
										// has spent executing in Kernel mode (including all threads
										// in all processes, on all processors). This time value also
										// includes the amount of time the system has been idle.
	ULARGE_INTEGER mPrevSysUserTime;	// the amount of time previously measured that the system
										// has spent executing in User mode (including all threads
										// in all processes, on all processors).
	ULARGE_INTEGER mPrevProcKernelTime;	// the amount of time that the process has executed in kernel mode
	ULARGE_INTEGER mPrevProcUserTime;	// the amount of time that the process has executed in user mode
#elif defined(__linux__)
	long mJiffiesPerSecond;
	unsigned long long mStartTimeSinceBoot;
	unsigned long long mPrevSystemTime;
	unsigned long long mPrevProcUserTime;
	unsigned long long mPrevProcKernelTime;
#endif
};

#endif